#include "debug.h"
#include "driver/ledc.h"
#include "mqtt.h"
#include "pins.h"
#include <Arduino.h>
#include <WiFi.h>
#include <espnow.h>

#define WAIT_TIME_ms 2000 // How long to wait for the RC controller to connect
#ifdef UBC
const char *ssid = "SharkTank";
const char *password = "nurgi2021";
IPAddress local_IP(192, 168, 137, IP_ADDR_LAST_OCTET);
IPAddress gateway(192, 168, 137, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   // optional
IPAddress secondaryDNS(8, 8, 4, 4); // optional
#else
const char *ssid = "ORBI27";
const char *password = "UBC2022!";
IPAddress local_IP(10, 0, 0, IP_ADDR_LAST_OCTET);
IPAddress gateway(10, 0, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);
#endif

// WIFI config

// MQTT Config
#ifdef IMAC
const char *mqtt_server =
    "10.0.0.26"; //"192.168.137.141"; // PC IP running Mosquitto
#endif

#ifdef MQTT
const int mqtt_port = 1883;
const char *deviceId = DEVICE_ID;
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long callback_count = 0;
unsigned long last_report_time = 0;
unsigned long total_callback_time = 0;
unsigned long max_callback_time = 0;
unsigned long loop_count = 0;
bool performance_monitoring = true;
#endif

int zero_steer_digital = 135; // This corresponds to about 1.7V Important: This
                              // must be the same as the python config
int zero_throttle_digital = 135; // This corresponds to about 1.7V Important:
                                 // This must be the same as the python config

int commanded_throttle = zero_throttle_digital; // throttle received over serial
int steering = zero_steer_digital;

void pair_with_car() {
  // Dummy function to illustrate the use of delay()
  dacWrite(DAC_STEERING_PIN, zero_steer_digital);
  dacWrite(DAC_THROTTLE_PIN, zero_throttle_digital);
}

void initWiFi() {
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.mode(WIFI_STA);
  Serial.print(WiFi.macAddress());
#ifdef MQTT
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  WiFi.setSleep(false);
  int rssi = WiFi.RSSI();
  Serial.printf("WiFi Signal: %d dBm\n", rssi);
#endif
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pair_with_car();
  // Wifi setup
  initWiFi();
#ifdef MQTT
  // MQTT setup
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
#endif
#ifdef ESPNOW
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  addPeer(broadcastAddress);
#endif
}

/**
 * @brief Reads the throttle value from the command string.
 *
 * The command string is expected to be in the format "t:<value>".
 * Returns the throttle value as an integer, or -1 if the format is incorrect
 * (does not start with "t:").
 * @param cmd The command string to parse. We expect the format to be
 * "t:<value>", where value is 0-255.
 * @return int The parsed throttle value, or -1 if parsing fails (does not start
 * with "t:").
 */

int readThrottle(String cmd) {
  if (cmd.startsWith("t:")) {
    String val = cmd.substring(2);
    return val.toInt();
  }
  return -1;
}

int readSteering(String cmd) {
  if (cmd.startsWith("s:")) {
    String val = cmd.substring(2);
    return val.toInt();
  }
  return -1;
}

void loop() {

#ifdef MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
#endif
#ifdef ESPNOW
  // Receiving a command from laptop:
  if (Serial.available()) {

    String cmd = Serial.readStringUntil('\n');
    cmd.trim(); // remove any whitespace/newline characters
    DEBUG_PRINTLN("ESP: Received command: ");
    DEBUG_PRINTLN(cmd);

    int speed = readThrottle(cmd);
    if (speed != -1) {
      commanded_throttle = speed;
    }
    int steer = readSteering(cmd);
    if (steer != -1) {
      steering = steer;
    }
    if (speed != -1 || steer != -1) {
      struct_message outgoing_msg;
      strcpy(outgoing_msg.msg, "CMD");
      outgoing_msg.throttle = commanded_throttle;
      outgoing_msg.steering = steering;
      outgoing_msg.send = true;

      // Send to broadcast address (or specific peer)
      // TODO: Update to add peer address in the serial command
      esp_err_t result = esp_now_send(
          broadcastAddress, (uint8_t *)&outgoing_msg, sizeof(outgoing_msg));
#ifdef DEBUG
      if (result == ESP_OK) {
        DEBUG_PRINTLN("ESP-NOW: Message sent successfully");
      } else {
        DEBUG_PRINTLN("ESP-NOW: Error sending message");
      }
#endif
    }
  }
#endif
}

#include "driver/ledc.h"
#include <Arduino.h>
#include <WiFi.h>

#include "mqtt.h"
#include "pins.h"

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
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);

  pair_with_car();
  // Wifi setup
  initWiFi();
  // MQTT setup
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  Serial.println("\n===== ESP32 RC Car Controller =====");
  Serial.printf("Device ID: %s\n", deviceId);
  Serial.printf("MQTT Server: %s:%d\n", mqtt_server, mqtt_port);
  Serial.println("Performance monitoring: ENABLED");
  Serial.println("===================================\n");
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

  if (!client.connected())
    reconnect();
  client.loop();

  if (performance_monitoring) {
    loop_count++;

    // Report performance every 5 seconds
    unsigned long now = millis();
    if (now - last_report_time >= 5000) {
      if (callback_count > 0) {
        float avg_time = total_callback_time / (float)callback_count;
        float messages_per_sec = callback_count / 5.0;
        float loop_freq = loop_count / 5.0;

        Serial.println("\n===== PERFORMANCE REPORT =====");
        Serial.printf("Messages received: %lu (%.1f msg/sec)\n", callback_count,
                      messages_per_sec);
        Serial.printf("Avg callback time: %.2f us\n", avg_time);
        Serial.printf("Max callback time: %lu us\n", max_callback_time);
        Serial.printf("Loop frequency: %.1f Hz\n", loop_freq);

        // Status assessment
        if (messages_per_sec >= 45) {
          Serial.println("Status: EXCELLENT - Handling 50+ FPS");
        } else if (messages_per_sec >= 30) {
          Serial.println("Status: GOOD - Smooth control");
        } else if (messages_per_sec >= 20) {
          Serial.println("Status: OK - Acceptable");
        } else if (messages_per_sec >= 5) {
          Serial.println("Status: LOW - Check Python polling rate");
        } else {
          Serial.println("Status: WARNING - Very low message rate");
        }

        // Loop frequency warning
        if (loop_freq < 1000) {
          Serial.printf("WARNING: Loop frequency low (%.1f Hz)\n", loop_freq);
          Serial.println("Check for blocking code in loop()");
        }

        // Latency estimate
        if (avg_time < 500) {
          Serial.println("Callback latency: Excellent (<0.5ms)");
        } else if (avg_time < 1000) {
          Serial.println("Callback latency: Good (<1ms)");
        } else {
          Serial.println("Callback latency: Consider optimization");
        }

        Serial.println("==============================\n");
      } else {
        Serial.println("No messages received in last 5 seconds");
      }

      // Reset counters
      callback_count = 0;
      total_callback_time = 0;
      max_callback_time = 0;
      loop_count = 0;
      last_report_time = now;
    }
  }

  // Receiving a command from laptop:
  //  if (Serial.available()) {
  //
  //    String cmd = Serial.readStringUntil('\n');
  //    cmd.trim(); // remove any whitespace/newline characters
  //    Serial.print("ESP: Received command: ");
  //    Serial.println(cmd);
  //
  //    int speed = readThrottle(cmd);
  //    if (speed != -1) {
  //      commanded_throttle = speed;
  //      dacWrite(DAC_THROTTLE_PIN, commanded_throttle);
  //      Serial.print("ESP: Updated throttle to ");
  //      Serial.println(commanded_throttle);
  //    }
  //    int steer = readSteering(cmd);
  //    if (steer != -1) {
  //      steering = steer;
  //      dacWrite(DAC_STEERING_PIN, steering);
  //      Serial.print("ESP: Updated steering to ");
  //      Serial.println(steering);
  //    }
  //  }
}

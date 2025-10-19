#include <Arduino.h>
#include "driver/ledc.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "mqtt.h"
#include "pins.h"


#define WAIT_TIME_ms 2000 //How long to wait for the RC controller to connect
#ifdef UBC
const char* ssid = "UBCSECURE";
const char* password = "......"; 
#else
const char* ssid = "ORBI27";
const char* password = "UBC2022!"; 
#endif 

//WIFI config 
IPAddress local_IP(10, 0, 0, IP_ADDR_LAST_OCTET);
IPAddress gateway(10, 0, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   // optional
IPAddress secondaryDNS(8, 8, 4, 4); // optional

//MQTT Config
#ifdef IMAC
const char* mqtt_server = "10.0.0.73"; // PC IP running Mosquitto
#endif
const int mqtt_port = 1883;
const char* deviceId = DEVICE_ID;
WiFiClient espClient;
PubSubClient client(espClient);
 

int zero_steer_digital = 119; //This corresponds to about 1.55V
int zero_throttle_digital = 119; //This corresponds to about 1.55V

int commanded_throttle = zero_throttle_digital; // throttle received over serial
int steering = zero_steer_digital;

void pair_with_car() {
  // Dummy function to illustrate the use of delay()
  dacWrite(DAC_STEERING_PIN, zero_steer_digital);
  dacWrite(DAC_THROTTLE_PIN, zero_throttle_digital);
  delay(10000);
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
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  
  pair_with_car();
  //Wifi setup 
  initWiFi();
  // MQTT setup
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

/**
 * @brief Reads the throttle value from the command string.
 * 
 * The command string is expected to be in the format "t:<value>".
 * Returns the throttle value as an integer, or -1 if the format is incorrect (does not start with "t:").
 * @param cmd The command string to parse. We expect the format to be "t:<value>", where value is 0-255.
 * @return int The parsed throttle value, or -1 if parsing fails (does not start with "t:").
 */

int readThrottle(String cmd){

  if (cmd.startsWith("t:")){
    String val = cmd.substring(2);
    return val.toInt();
  }

  return -1;
  
}


int readSteering(String cmd){

  if (cmd.startsWith("s:")){
    String val = cmd.substring(2);
    return val.toInt();
  }
  
  return -1;
}

void loop() {

  if (!client.connected()) reconnect();
  client.loop();

  // Receiving a command from laptop:
  if (Serial.available()) {

    String cmd = Serial.readStringUntil('\n');
    cmd.trim(); // remove any whitespace/newline characters
    Serial.print("ESP: Received command: ");
    Serial.println(cmd);

    int speed = readThrottle(cmd);
    if (speed != -1) {
      commanded_throttle = speed;
      dacWrite(DAC_THROTTLE_PIN, commanded_throttle);
      Serial.print("ESP: Updated throttle to ");
      Serial.println(commanded_throttle);
    }
    int steer = readSteering(cmd);
    if (steer != -1) {
      steering = steer;
      dacWrite(DAC_STEERING_PIN, steering);
      Serial.print("ESP: Updated steering to ");
      Serial.println(steering);
    }
  }
}



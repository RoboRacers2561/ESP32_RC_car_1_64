#include "debug.h"
#include "driver/ledc.h"
#include "pins.h"
#include <Arduino.h>
#include <WiFi.h>
#include <espnow.h>

#define WAIT_TIME_ms 2000 // How long to wait for the RC controller to connect

IPAddress local_IP(10, 0, 0, IP_ADDR_LAST_OCTET);
IPAddress gateway(10, 0, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

#if TTR
int zero_steer_digital = 141;
int zero_throttle_digital = 122;
#else
int zero_steer_digital = 135;
int zero_throttle_digital = 135;
#endif
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
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pair_with_car();
  // Wifi setup
  initWiFi();
#if ESPNOW
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

#if ESPNOW
  // Receiving a command from laptop:
  if (Serial.available()) {

    String msg = Serial.readStringUntil('\n');
    uint32_t idx = msg.indexOf('/');
    String id = msg.substring(0, idx);
    uint8_t byteAddress[6];
    stringToBytes(id.c_str(), byteAddress, 6);
    String cmd = msg.substring(idx + 1);
    cmd.trim(); // remove any whitespace/newline characterso
    if (!checkPeer(byteAddress)) {
      addPeer(byteAddress);
    }
    DEBUG_PRINTLN("ESP: Received command: ");
    DEBUG_PRINTLN(cmd);
    struct_message outgoing_msg;
    int speed = readThrottle(cmd);
    if (speed != -1) {
      outgoing_msg.value = speed;
      outgoing_msg.throttle = 1;
    }
    int steer = readSteering(cmd);
    if (steer != -1) {
      outgoing_msg.value = steer;
      outgoing_msg.throttle = 0;
    }
    if (speed != -1 || steer != -1) {
      if (checkPeer(byteAddress)) {
        esp_err_t result = esp_now_send(byteAddress, (uint8_t *)&outgoing_msg,
                                        sizeof(outgoing_msg));
#if DEBUG
        if (result == ESP_OK) {
          DEBUG_PRINTLN("ESP-NOW: Message sent successfully");
        } else {
          DEBUG_PRINTLN("ESP-NOW: Error sending message");
        }
#endif
      }
    }
  }
#endif
}

#include "mqtt.h"

// Networking callbacks for MQTT command parsing
void callback(char *topic, byte *payload, unsigned int length) {
  // Make payload null-terminated for direct use (FAST)
  digitalWrite(LED_PIN, HIGH);
  unsigned long start_time = micros();
  payload[length] = '\0';
  char *msg = (char *)payload;

  // Quick topic parsing - find last '/' character
  char *lastSlash = strrchr(topic, '/');
  if (!lastSlash)
    return;
  lastSlash++; // Move past the '/'

  // Use switch-case with first character for fastest branching
  switch (lastSlash[0]) {
  case 'l': // "led"
    if (strcmp(lastSlash, "led") == 0) {
      // Compare first char only for speed
      if (msg[4] == 'O' && msg[5] == 'N') { // "LED_ON"
        digitalWrite(LED_PIN, HIGH);
      } else if (msg[4] == 'O' && msg[5] == 'F') { // "LED_OFF"
        digitalWrite(LED_PIN, LOW);
      }
    }
    break;

  case 't':                    // "throttle" or "time"
    if (lastSlash[1] == 'h') { // "throttle"
      // Direct ASCII to int conversion (faster than atoi)
      int speed = 0;
      for (unsigned int i = 0; i < length; i++) {
        if (msg[i] >= '0' && msg[i] <= '9') {
          speed = speed * 10 + (msg[i] - '0');
        }
      }
      commanded_throttle = speed;
      dacWrite(DAC_THROTTLE_PIN, commanded_throttle);
    } else if (lastSlash[1] == 'i') { // "time"
      // Handle timing test - echo back with timestamp
      String response_topic = "devices/" + String(deviceId) + "/echo/time";
      client.publish(response_topic.c_str(), "echo");
    }
    break;

  case 's': // "steering"
    if (strcmp(lastSlash, "steering") == 0) {
      // Direct ASCII to int conversion
      int steer = 0;
      bool negative = (msg[0] == '-');
      unsigned int start = negative ? 1 : 0;

      for (unsigned int i = start; i < length; i++) {
        if (msg[i] >= '0' && msg[i] <= '9') {
          steer = steer * 10 + (msg[i] - '0');
        }
      }
      if (negative)
        steer = -steer;

      steering = steer;
      dacWrite(DAC_STEERING_PIN, steering);
    }
    break;
  }
  if (performance_monitoring) {
    unsigned long callback_time = micros() - start_time;
    total_callback_time += callback_time;
    if (callback_time > max_callback_time) {
      max_callback_time = callback_time;
    }
    callback_count++;
  }
  digitalWrite(LED_PIN, LOW);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(deviceId)) {
      client.subscribe(("devices/" + String(deviceId) + "/set/led").c_str());
      client.subscribe(
          ("devices/" + String(deviceId) + "/set/throttle").c_str());
      client.subscribe(
          ("devices/" + String(deviceId) + "/set/steering").c_str());
      client.subscribe(("devices/" + String(deviceId) + "/set/time").c_str());

      client.publish(("devices/" + String(deviceId) + "/status").c_str(),
                     "online");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retry in 2s");
      delay(2000);
    }
  }
}

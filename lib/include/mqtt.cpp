#include "mqtt.h"

//Networking callbacks for MQTT command parsing 
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.printf("Received [%s]: %s\n", topic, msg.c_str());
  // Example: handle command

  if (String(topic).endsWith("/led")) {
    if(msg == "LED_ON") digitalWrite(LED_PIN, HIGH);
    if(msg == "LED_OFF") digitalWrite(LED_PIN, LOW);
  }
  // Handle throttle commands
  else if (String(topic).endsWith("/throttle")) {
    int speed = msg.toInt();
    commanded_throttle = speed;
    dacWrite(DAC_THROTTLE_PIN, commanded_throttle);
    Serial.printf("Throttle set via MQTT: %d\n", commanded_throttle);
  }

  // Handle steering commands
  else if (String(topic).endsWith("/steering")) {
    int steer = msg.toInt();
    steering = steer;
    dacWrite(DAC_STEERING_PIN, steering);
    Serial.printf("Steering set via MQTT: %d\n", steering);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(deviceId)) {
      client.subscribe(("devices/" + String(deviceId) + "/set/led").c_str());
      client.subscribe(("devices/" + String(deviceId) + "/set/throttle").c_str());
      client.subscribe(("devices/" + String(deviceId) + "/set/steering").c_str());
      client.publish(("devices/" + String(deviceId) + "/status").c_str(), "online");
    } else {
      delay(2000);
    }
  }
}

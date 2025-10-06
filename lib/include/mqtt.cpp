#include "mqtt.h"
//Networking callbacks for MQTT command parsing 
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.printf("Received [%s]: %s\n", topic, msg.c_str());
  // Example: handle command
  if(msg == "LED_ON") digitalWrite(led_pin, HIGH);
  if(msg == "LED_OFF") digitalWrite(led_pin, LOW);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(deviceId)) {
      client.subscribe(("devices/" + String(deviceId) + "/set").c_str());
      client.publish(("devices/" + String(deviceId) + "/status").c_str(), "online");
    } else {
      delay(2000);
    }
  }
}

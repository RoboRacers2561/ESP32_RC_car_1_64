#include "mqtt.h"

//Networking callbacks for MQTT command parsing 
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.printf("Received [%s]: %s\n", topic, msg.c_str());
  

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
  // Handle steering commands
  else if (String(topic).endsWith("/steering")) {
    int steer = msg.toInt();
    steering = steer;
    dacWrite(DAC_STEERING_PIN, steering);
    Serial.printf("Steering set via MQTT: %d\n", steering);
  }
  // Handle timing test messages
  else if (String(topic).endsWith("/time")) {
    // Parse JSON payload for timing test
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, msg);
    
    if (error) {
      Serial.print("JSON parsing failed: ");
      Serial.println(error.c_str());
      return;
    }
    
    double t_send = doc["t_send"];
    
    // Echo back the timestamp immediately
    StaticJsonDocument<200> response;
    response["t_send"] = t_send;
    response["t_recv"] = millis() / 1000.0;  // ESP32 receive time
    
    char buffer[200];
    serializeJson(response, buffer);
    
    String response_topic = "devices/" + String(deviceId) + "/echo/time";
    client.publish(response_topic.c_str(), buffer);
    Serial.printf("Echoed timing message: %s\n", buffer);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(deviceId)) {
      client.subscribe(("devices/" + String(deviceId) + "/set/led").c_str());
      client.subscribe(("devices/" + String(deviceId) + "/set/throttle").c_str());
      client.subscribe(("devices/" + String(deviceId) + "/set/steering").c_str());
      client.subscribe(("devices/" + String(deviceId) + "/set/time").c_str());
     
      client.publish(("devices/" + String(deviceId) + "/status").c_str(), "online");
} else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retry in 2s");
      delay(2000);
    }
  }
}

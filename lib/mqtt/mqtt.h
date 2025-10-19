//Networking callbacks for MQTT command parsing 
#include <PubSubClient.h>
#include <pins.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
extern const char* deviceId;
extern PubSubClient client;
extern int commanded_throttle; 
extern int steering;           


void callback(char* topic, byte* payload, unsigned int length);
void reconnect();

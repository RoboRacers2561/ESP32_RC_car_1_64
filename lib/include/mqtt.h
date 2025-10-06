//Networking callbacks for MQTT command parsing 
#include <PubSubClient.h>


extern const int led_pin;
extern const char* deviceId;
extern PubSubClient client;

void callback(char* topic, byte* payload, unsigned int length);
void reconnect();

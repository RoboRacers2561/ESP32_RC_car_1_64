// Networking callbacks for MQTT command parsing
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <pins.h>
extern const char *deviceId;
extern PubSubClient client;
extern int commanded_throttle;
extern int steering;
extern unsigned long callback_count;
extern unsigned long total_callback_time;
extern unsigned long max_callback_time;
extern bool performance_monitoring;

void callback(char *topic, byte *payload, unsigned int length);
void reconnect();

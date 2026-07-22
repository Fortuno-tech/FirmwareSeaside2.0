#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <PubSubClient.h>

extern PubSubClient mqttClient;

void setupMQTT(const char* ssid, const char* password);
void handleMQTT();
void mqttPublishCount(int total, int current);
void mqttPublishEntry();
void mqttPublishSlaveTelemetry(const char* slaveId, const char* mac, int count, int seuil, bool active);
void mqttPublishAlert(const String& message);

#endif

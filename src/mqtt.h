#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>

void setupMQTT(const char* ssid, const char* password);
void handleMQTT();
void mqttPublishCount(int total, int current);

#endif
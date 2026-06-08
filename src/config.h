#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

extern String apSSID;
extern String apPassword;
extern int totalPersonnes;
extern int personnesActuelles;
extern String moduleRole;
extern String masterMAC;

// WiFi STA
extern String staSSID;
extern String staPassword;

// MQTT
extern String mqttServer;
extern int mqttPort;

#endif
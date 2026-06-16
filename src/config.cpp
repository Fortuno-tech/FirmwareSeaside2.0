#include "config.h"

String apSSID      = "SmartCount";
String apPassword  = "Fortico1234";

int totalPersonnes     = 0;
int personnesActuelles = 0;

String moduleRole = "neutral";
String masterMAC  = "00:00:00:00:00:00";

// WiFi STA
String staSSID     = "";
String staPassword = "";

// MQTT
String mqttServer = "192.168.1.2";
int mqttPort      = 1883;

bool mqttTriggerSetup = false;

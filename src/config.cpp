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
String mqttServer = "53cc1d1dc297463f9f511baf26ee908e.s1.eu.hivemq.cloud";
int mqttPort      = 8883;
String mqttUser     = "Fortico";
String mqttPassword = "Fortico123456";

bool mqttTriggerSetup = false;
bool requestReboot = false;
unsigned long rebootTimer = 0;

// Seuil de détection capteur ultrason (cm)
int seuil = SEUIL_DEFAULT;

// Identifiant unique du module et flag de configuration
String moduleId          = "Master";
bool   isMasterConfigured = false;
String licenseCode        = "";

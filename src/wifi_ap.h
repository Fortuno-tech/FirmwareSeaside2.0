#ifndef WIFI_AP_H
#define WIFI_AP_H

#include <Arduino.h>

void setupAP();
void modifierAP(String newSSID, String newPassword);
void handleSlaveAnnounce();

#endif
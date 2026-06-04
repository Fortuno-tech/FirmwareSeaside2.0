#include "wifi_ap.h"
#include "config.h"
#include <WiFi.h>

void setupAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID.c_str(), apPassword.c_str());
  Serial.print("AP démarré : ");
  Serial.println(apSSID);
  Serial.print("IP : ");
  Serial.println(WiFi.softAPIP());
}

void modifierAP(String newSSID, String newPassword) {
  apSSID     = newSSID;
  apPassword = newPassword;
  WiFi.softAP(apSSID.c_str(), apPassword.c_str());
  Serial.println("AP mis à jour !");
  Serial.print("Nouveau SSID : ");
  Serial.println(apSSID);
}

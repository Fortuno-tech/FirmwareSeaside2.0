#include "wifi_ap.h"
#include "config.h"

#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif

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
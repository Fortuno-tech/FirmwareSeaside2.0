#include "wifi_ap.h"
#include "config.h"
#include <WiFi.h>

void setupAP() {
  // Si apSSID est vide ou a sa valeur par défaut, générer avec le suffixe MAC
  if (apSSID == "SmartCount" || apSSID == "") {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    String suffix = mac.substring(mac.length() - 3);
    apSSID = "SmartCount-" + suffix;
  }

  WiFi.mode(WIFI_AP_STA);
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


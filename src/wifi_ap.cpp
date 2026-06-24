#include "wifi_ap.h"
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

void setupAP() {
  // Si apSSID est vide ou a sa valeur par défaut, générer avec le suffixe MAC
  if (apSSID == "SmartCount" || apSSID == "") {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    String suffix = mac.substring(mac.length() - 3);
    apSSID = "SmartCount-" + suffix;
  }

  if (moduleRole == "slave") {
    // Si c'est un esclave, pas d'AP local ! Mode station (STA) uniquement
    WiFi.mode(WIFI_STA);
    if (staSSID != "") {
      Serial.print("WiFi STA (Slave) : Tentative de connexion au Master AP ");
      Serial.println(staSSID);
      WiFi.begin(staSSID.c_str(), staPassword.c_str());
    } else {
      Serial.println("WiFi STA (Slave) : Erreur, aucun SSID Master configuré !");
    }
  } else {
    // Si Master ou Neutral, on démarre l'AP local pour liaison
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(apSSID.c_str(), apPassword.c_str());
    Serial.print("AP démarré (");
    Serial.print(moduleRole);
    Serial.print(") : ");
    Serial.println(apSSID);
    Serial.print("IP AP : ");
    Serial.println(WiFi.softAPIP());
  }
}

void modifierAP(String newSSID, String newPassword) {
  apSSID     = newSSID;
  apPassword = newPassword;
  WiFi.softAP(apSSID.c_str(), apPassword.c_str());
  Serial.println("AP mis à jour !");
  Serial.print("Nouveau SSID : ");
  Serial.println(apSSID);
}

void handleSlaveAnnounce() {
  if (moduleRole != "slave") return;
  if (WiFi.status() != WL_CONNECTED) return;

  unsigned long now = millis();
  static unsigned long lastAnnounce = 0;

  if (now - lastAnnounce >= 10000 || lastAnnounce == 0) {
    lastAnnounce = now;
    
    IPAddress gateway = WiFi.gatewayIP();
    if (gateway.toString() == "0.0.0.0") return;

    WiFiClient client;
    HTTPClient http;
    String url = "http://" + gateway.toString() + "/api/register_slave";
    
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(2000); // 2 s timeout non bloquant

    StaticJsonDocument<256> doc;
    doc["mac"]      = WiFi.macAddress();
    doc["ip"]       = WiFi.localIP().toString();
    doc["moduleId"] = moduleId;
    doc["count"]    = compteur;
    doc["seuil"]    = seuil;

    String payload;
    serializeJson(doc, payload);

    int httpCode = http.POST(payload);
    if (httpCode > 0) {
      Serial.printf("[Slave] Annonce envoyée. Code: %d\n", httpCode);
    } else {
      Serial.printf("[Slave] Annonce échouée: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  }
}


#include "webserver.h"
#include "config.h"
#include "wifi_ap.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

AsyncWebServer server(80);

void setupServer() {

  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.on("/api/count", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<200> doc;
    doc["total"]   = totalPersonnes;
    doc["current"] = personnesActuelles;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<200> doc;
    doc["mqtt"]   = "disconnected";
    doc["role"]   = moduleRole;
    doc["ip"]     = WiFi.softAPIP().toString();
    doc["ssid"]   = apSSID;
    doc["mac"]    = WiFi.macAddress();
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/api/config/module", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<200> doc;
      deserializeJson(doc, data, len);

      if(doc.containsKey("role")) moduleRole = doc["role"].as<String>();
      if(doc.containsKey("masterMAC")) masterMAC = doc["masterMAC"].as<String>();

      Serial.println("Nouvelle config module reçue !");
      Serial.print("Rôle: "); Serial.println(moduleRole);
      Serial.print("Master MAC: "); Serial.println(masterMAC);

      request->send(200, "application/json", "{\"status\":\"ok\"}");
      
      // Optionnel: Redémarrer pour appliquer les changements proprement
      // delay(1000); ESP.restart(); 
    }
  );

  server.on("/api/config/ap", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<200> doc;
      deserializeJson(doc, data, len);

      String newSSID     = doc["ssid"]     | apSSID;
      String newPassword = doc["password"] | apPassword;

      if (newSSID.length() < 1 || newPassword.length() < 8) {
        request->send(400, "application/json", "{\"error\":\"SSID vide ou MDP trop court\"}");
        return;
      }

      modifierAP(newSSID, newPassword);
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }
  );

  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "application/json", "{\"error\":\"Not found\"}");
  });

  server.begin();
  Serial.println("Serveur démarré !");
}

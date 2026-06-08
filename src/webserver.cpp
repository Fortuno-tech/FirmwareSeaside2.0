#include "webserver.h"
#include "config.h"
#include "wifi_ap.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Update.h>

AsyncWebServer server(80);

void setupServer() {
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // GET /api/status
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<256> doc;
    doc["total"]   = totalPersonnes;
    doc["current"] = personnesActuelles;
    doc["role"]    = moduleRole;
    doc["ip"]      = WiFi.softAPIP().toString();
    doc["mac"]     = WiFi.macAddress();
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // GET /api/count
  server.on("/api/count", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<200> doc;
    doc["total"]   = totalPersonnes;
    doc["current"] = personnesActuelles;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // POST /api/config/module
  server.on("/api/config/module", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<256> doc;
      deserializeJson(doc, data, len);
      if (doc.containsKey("role"))      moduleRole = doc["role"].as<String>();
      if (doc.containsKey("masterMAC")) masterMAC  = doc["masterMAC"].as<String>();
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }
  );

  // POST /api/config/ap
  server.on("/api/config/ap", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<256> doc;
      deserializeJson(doc, data, len);
      String newSSID = doc["ssid"]     | apSSID;
      String newPass = doc["password"] | apPassword;
      modifierAP(newSSID, newPass);
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }
  );

  // GET /api/wifi
  server.on("/api/wifi", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<200> doc;
    doc["ssid"]     = staSSID;
    doc["password"] = staPassword;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // POST /api/wifi
  server.on("/api/wifi", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<200> doc;
      deserializeJson(doc, data, len);
      String newSSID     = doc["ssid"]     | staSSID;
      String newPassword = doc["password"] | staPassword;
      if (newSSID.length() < 1) {
        request->send(400, "application/json", "{\"error\":\"SSID vide\"}");
        return;
      }
      staSSID     = newSSID;
      staPassword = newPassword;
      request->send(200, "application/json", "{\"status\":\"ok\"}");
      Serial.println("WiFi STA mis à jour !");
    }
  );

  // GET /api/mqtt
  server.on("/api/mqtt", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<200> doc;
    doc["server"] = mqttServer;
    doc["port"]   = mqttPort;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // POST /api/mqtt
  server.on("/api/mqtt", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<200> doc;
      deserializeJson(doc, data, len);
      String newServer = doc["server"] | mqttServer;
      int    newPort   = doc["port"]   | mqttPort;
      if (newServer.length() < 1) {
        request->send(400, "application/json", "{\"error\":\"Serveur MQTT vide\"}");
        return;
      }
      mqttServer = newServer;
      mqttPort   = newPort;
      request->send(200, "application/json", "{\"status\":\"ok\"}");
      Serial.println("MQTT mis à jour !");
    }
  );

  // OTA Upload
  server.on("/update", HTTP_POST,
    [](AsyncWebServerRequest* request) {
      bool success = !Update.hasError();
      request->send(200, "text/plain", success ? "OK" : "FAILED");
      if (success) {
        delay(500);
        ESP.restart();
      }
    },
    [](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
      if (index == 0) {
        Serial.printf("OTA Start: %s\n", filename.c_str());
        Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000);
      }
      Update.write(data, len);
      if (final) {
        Update.end(true);
        Serial.println("OTA terminé !");
      }
    }
  );

  // 404
  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "application/json", "{\"error\":\"Not found\"}");
  });

  server.begin();
  Serial.println("Serveur démarré !");
}
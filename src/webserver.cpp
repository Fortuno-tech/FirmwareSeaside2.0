#include "webserver.h"
#include "config.h"
#include "wifi_ap.h"
#include "storage.h"
#include "mqtt.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Update.h>


AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void setupServer() {
  // Configurer le WebSocket
  ws.onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
      Serial.printf("WebSocket client #%u connecté\n", client->id());
      int valToShow = (moduleRole == "master") ? totalPersonnes : compteur;
      client->text(String(valToShow));
    } else if (type == WS_EVT_DISCONNECT) {
      Serial.printf("WebSocket client #%u déconnecté\n", client->id());
    }
  });
  server.addHandler(&ws);

  // ─── GET /api/status ──────────────────────────────────────────────────────
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<256> doc;
    doc["total"]     = totalPersonnes;
    doc["current"]   = personnesActuelles;
    doc["role"]      = moduleRole;
    doc["ip"]        = WiFi.softAPIP().toString();
    doc["mac"]       = WiFi.macAddress();
    doc["connected"] = (WiFi.status() == WL_CONNECTED);
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // ─── GET /api/count ───────────────────────────────────────────────────────
  server.on("/api/count", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<200> doc;
    doc["total"]    = totalPersonnes;
    doc["current"]  = personnesActuelles;
    doc["compteur"] = compteur;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // ─── POST /api/count ──────────────────────────────────────────────────────
  server.on("/api/count", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<200> doc;
      deserializeJson(doc, data, len);
      if (doc.containsKey("count")) {
        int newVal = doc["count"].as<int>();
        compteur           = newVal;
        totalPersonnes     = newVal;
        personnesActuelles = newVal;
        storage_markDirty();
        webserver_broadcastCount(newVal);
      }
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }
  );

  // ─── GET /api/config/module ───────────────────────────────────────────────
  server.on("/api/config/module", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<256> doc;
    doc["role"]      = moduleRole;
    doc["masterMAC"] = masterMAC;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // ─── POST /api/config/module ──────────────────────────────────────────────
  server.on("/api/config/module", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<256> doc;
      deserializeJson(doc, data, len);
      if (doc.containsKey("role"))      moduleRole = doc["role"].as<String>();
      if (doc.containsKey("type"))      moduleRole = doc["type"].as<String>();
      moduleRole.toLowerCase();
      if (doc.containsKey("masterMAC")) masterMAC = doc["masterMAC"].as<String>();
      if (doc.containsKey("macMaster")) masterMAC = doc["macMaster"].as<String>();
      storage_saveConfig();
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }
  );

  // ─── GET /api/config/ap ───────────────────────────────────────────────────
  server.on("/api/config/ap", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<256> doc;
    doc["ssid"]     = apSSID;
    doc["password"] = apPassword;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // ─── POST /api/config/ap ──────────────────────────────────────────────────
  server.on("/api/config/ap", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<256> doc;
      deserializeJson(doc, data, len);
      String newSSID = doc["ssid"]     | apSSID;
      String newPass = doc["password"] | apPassword;
      modifierAP(newSSID, newPass);
      storage_saveConfig();
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }
  );

  // ─── GET /api/wifi ────────────────────────────────────────────────────────
  server.on("/api/wifi", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<200> doc;
    doc["ssid"]     = staSSID;
    doc["password"] = staPassword;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // ─── POST /api/wifi ───────────────────────────────────────────────────────
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
      storage_saveConfig();
      mqttTriggerSetup = true;
      request->send(200, "application/json", "{\"status\":\"ok\"}");
      Serial.println("WiFi STA mis à jour !");
    }
  );

  // ─── GET /api/mqtt ────────────────────────────────────────────────────────
  server.on("/api/mqtt", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<300> doc;
    doc["server"]   = mqttServer;
    doc["port"]     = mqttPort;
    doc["user"]     = mqttUser;
    doc["password"] = mqttPassword;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // ─── POST /api/mqtt ───────────────────────────────────────────────────────
  server.on("/api/mqtt", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<300> doc;
      deserializeJson(doc, data, len);
      String newServer = doc["server"] | mqttServer;
      int    newPort   = doc["port"]   | mqttPort;
      String newUser   = doc["user"]   | mqttUser;
      String newPass   = doc["password"] | mqttPassword;
      if (newServer.length() < 1) {
        request->send(400, "application/json", "{\"error\":\"Serveur MQTT vide\"}");
        return;
      }
      mqttServer   = newServer;
      mqttPort     = newPort;
      mqttUser     = newUser;
      mqttPassword = newPass;
      storage_saveConfig();
      mqttTriggerSetup = true;
      request->send(200, "application/json", "{\"status\":\"ok\"}");
      Serial.println("MQTT mis à jour !");
    }
  );

  // ─── GET /api/config/sensor ───────────────────────────────────────────────
  // Retourne le seuil de détection actuel et ses limites
  server.on("/api/config/sensor", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<128> doc;
    doc["seuil"]    = seuil;
    doc["seuilMin"] = SEUIL_MIN;
    doc["seuilMax"] = SEUIL_MAX;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // ─── POST /api/config/sensor ──────────────────────────────────────────────
  // Modifie le seuil de détection en temps réel et le sauvegarde en flash
  server.on("/api/config/sensor", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<128> doc;
      deserializeJson(doc, data, len);
      if (!doc.containsKey("seuil")) {
        request->send(400, "application/json", "{\"error\":\"Champ seuil manquant\"}");
        return;
      }
      int newSeuil = doc["seuil"].as<int>();
      if (newSeuil < SEUIL_MIN || newSeuil > SEUIL_MAX) {
        request->send(400, "application/json", "{\"error\":\"Valeur hors limites (10-500 cm)\"}");
        return;
      }
      seuil = newSeuil;
      storage_saveConfig();
      Serial.printf("✓ Seuil de détection mis à jour : %d cm\n", seuil);
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }
  );

  // ─── OTA Upload ───────────────────────────────────────────────────────────
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

  // ─── Fichiers statiques ───────────────────────────────────────────────────
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // ─── 404 ──────────────────────────────────────────────────────────────────
  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "application/json", "{\"error\":\"Not found\"}");
  });

  server.begin();
  Serial.println("Serveur démarré !");
}

void webserver_broadcastCount(int val) {
  ws.textAll(String(val));
}

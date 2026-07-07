#include "webserver.h"
#include "config.h"
#include "wifi_ap.h"
#include "storage.h"
#include "mqtt.h"
#include "espnow.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Update.h>
#include <vector>

// ─── Structures ───────────────────────────────────────────────────────────────

struct SlaveInfo {
  String mac;
  String ip;
  String moduleId;
  int    count;
  unsigned long lastSeen;
  int    seuil;
};

// Slaves auto-découverts (via HTTP announce depuis wifi_ap.cpp)
static std::vector<SlaveInfo> s_registeredSlaves;

// Slaves configurés manuellement depuis l'interface (liste persistante côté web)
struct ConfiguredSlave {
  String mac;
  String moduleId;   // Nom donné par l'utilisateur (ex: "Slave-1")
  String wifiSSID;
  String wifiPassword;
};
static std::vector<ConfiguredSlave> s_configuredSlaves;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ─────────────────────────────────────────────────────────────────────────────

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

// ─── POST /api/license/generate ──────────────────────────────────────────────
  server.on("/api/license/generate", HTTP_POST, [] (AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<256> doc;
      deserializeJson(doc, data, len);
      int duration = doc["durationDays"] | 30; // default 30 days
      if (storage_generateLicense(duration)) {
        StaticJsonDocument<256> resp;
        resp["licenseCode"] = licenseCode;
        resp["licenseDate"] = licenseDate;
        resp["licenseDuration"] = licenseDuration;
        resp["licenseExpiry"] = licenseExpiry;
        String out;
        serializeJson(resp, out);
        request->send(200, "application/json", out);
      } else {
        request->send(500, "application/json", "{\"error\":\"generation_failed\"}");
      }
    });

  // ─── POST /api/module/reset ─────────────────────────────────────────────────────
  server.on("/api/module/reset", HTTP_POST, [] (AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<256> doc;
      deserializeJson(doc, data, len);
      String targetId = doc["moduleId"].as<String>();
      if (targetId == moduleId) {
        // Reset master counters
        compteur = 0;
        totalPersonnes = 0;
        personnesActuelles = 0;
        storage_saveConfig();
        request->send(200, "application/json", "{\"status\":\"master_reset\"}");
      } else {
        // For slaves, send a reset command via MQTT (topic "seaside/command/reset/<moduleId>")
        String topic = "seaside/command/reset/" + targetId;
        if (mqttClient.connected()) {
          mqttClient.publish(topic.c_str(), "reset");
          request->send(200, "application/json", "{\"status\":\"reset_sent\"}");
        } else {
          request->send(503, "application/json", "{\"error\":\"mqtt_not_connected\"}");
        }
      }
    });

  // ─── POST /api/format ────────────────────────────────────────────────────────
  server.on("/api/format", HTTP_POST, [] (AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      storage_format();
      request->send(200, "application/json", "{\"status\":\"formatted\"}");
    });

  // Insert license validation check before publishing telemetry
  // (modify existing mqttPublishEntry below)

    StaticJsonDocument<512> doc;
    doc["total"]           = totalPersonnes;
    doc["current"]         = personnesActuelles;
    doc["role"]            = moduleRole;
    doc["moduleId"]        = moduleId;
    doc["ip"]              = (moduleRole == "slave") ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    doc["mac"]             = WiFi.macAddress();
    doc["connected"]       = (WiFi.status() == WL_CONNECTED);
    doc["battery"]         = 100;
    doc["licence"]         = licenseCode;
    doc["licenseDate"]     = licenseDate;
    doc["licenseDuration"] = licenseDuration;
    doc["licenseExpiry"]   = licenseExpiry;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // ─── POST /api/register_slave ──────────────────────────────────────────────
  server.on("/api/register_slave", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<256> doc;
      deserializeJson(doc, data, len);
      
      if (doc.containsKey("mac") && doc.containsKey("ip")) {
        String mac      = doc["mac"].as<String>();
        String ip       = doc["ip"].as<String>();
        String slvId    = doc["moduleId"].as<String>();
        int    count    = doc["count"] | 0;
        int    slvSeuil = doc["seuil"] | 80;
        
        bool found = false;
        for (auto& s : s_registeredSlaves) {
          if (s.mac.equalsIgnoreCase(mac)) {
            s.ip       = ip;
            s.count    = count;
            s.seuil    = slvSeuil;
            s.lastSeen = millis();
            if (slvId.length() > 0) s.moduleId = slvId;
            found = true;
            break;
          }
        }
        
        if (!found) {
          SlaveInfo s;
          s.mac      = mac;
          s.ip       = ip;
          s.moduleId = (slvId.length() > 0) ? slvId : ("Slave-" + String(s_registeredSlaves.size() + 1));
          s.count    = count;
          s.seuil    = slvSeuil;
          s.lastSeen = millis();
          s_registeredSlaves.push_back(s);
        }
        
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        request->send(400, "application/json", "{\"error\":\"Champs mac ou ip manquants\"}");
      }
    }
  );

  // ─── GET /api/slaves ───────────────────────────────────────────────────────
  // Retourne les slaves auto-découverts (via HTTP announce) + ceux reçus via ESP-NOW
  server.on("/api/slaves", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<2048> doc;
    JsonArray array = doc.to<JsonArray>();
    
    unsigned long now = millis();
    
    // Slaves HTTP (annonce WiFi)
    for (const auto& s : s_registeredSlaves) {
      JsonObject obj = array.createNestedObject();
      obj["mac"]      = s.mac;
      obj["ip"]       = s.ip;
      obj["moduleId"] = s.moduleId;
      obj["count"]    = s.count;
      obj["seuil"]    = s.seuil;
      obj["active"]   = (now - s.lastSeen < 30000);
      obj["source"]   = "wifi";
    }
    
    // Slaves ESP-NOW (envoi direct sans WiFi)
    int espNowCount = espnow_getSlaveCount();
    for (int i = 0; i < espNowCount; i++) {
      char macBuf[18], idBuf[20];
      int  cnt;
      bool active;
      int  slvSeuil;
      if (espnow_getSlaveInfo(i, macBuf, idBuf, &cnt, &active, &slvSeuil)) {
        // Vérifier qu'il n'est pas déjà dans la liste WiFi
        bool alreadyIn = false;
        for (const auto& s : s_registeredSlaves) {
          if (s.mac.equalsIgnoreCase(macBuf)) { alreadyIn = true; break; }
        }
        if (!alreadyIn) {
          JsonObject obj = array.createNestedObject();
          obj["mac"]      = String(macBuf);
          obj["ip"]       = "";
          obj["moduleId"] = String(idBuf[0] ? idBuf : ("Slave-" + String(i + 1)).c_str());
          obj["count"]    = cnt;
          obj["seuil"]    = slvSeuil;
          obj["active"]   = active;
          obj["source"]   = "espnow";
        }
      }
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // ─── GET /api/modules_count ────────────────────────────────────────────────
  // Retourne le compteur de chaque module (master + slaves) + le total général
  server.on("/api/modules_count", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<2048> doc;
    
    // Module master (ce module)
    JsonArray modules = doc.createNestedArray("modules");
    JsonObject masterObj = modules.createNestedObject();
    masterObj["moduleId"] = moduleId;
    masterObj["role"]     = moduleRole;
    masterObj["mac"]      = WiFi.macAddress();
    masterObj["count"]    = compteur;   // compteur local du master
    masterObj["seuil"]    = seuil;
    masterObj["active"]   = true;
    masterObj["battery"]  = 100;        // placeholder (ADC non câblé)
    masterObj["licence"]  = licenseCode;
    
    unsigned long now = millis();
    
    // Slaves HTTP
    for (const auto& s : s_registeredSlaves) {
      JsonObject obj = modules.createNestedObject();
      obj["moduleId"] = s.moduleId;
      obj["role"]     = "slave";
      obj["mac"]      = s.mac;
      obj["count"]    = s.count;
      obj["seuil"]    = s.seuil;
      obj["active"]   = (now - s.lastSeen < 30000);
    }
    
    // Slaves ESP-NOW (qui ne sont pas déjà dans HTTP)
    int espNowCount = espnow_getSlaveCount();
    for (int i = 0; i < espNowCount; i++) {
      char macBuf[18], idBuf[20];
      int  cnt;
      bool active;
      int  slvSeuil;
      if (espnow_getSlaveInfo(i, macBuf, idBuf, &cnt, &active, &slvSeuil)) {
        bool alreadyIn = false;
        for (const auto& s : s_registeredSlaves) {
          if (s.mac.equalsIgnoreCase(macBuf)) { alreadyIn = true; break; }
        }
        if (!alreadyIn) {
          JsonObject obj = modules.createNestedObject();
          obj["moduleId"] = String(idBuf[0] ? idBuf : ("Slave-" + String(i + 1)).c_str());
          obj["role"]     = "slave";
          obj["mac"]      = String(macBuf);
          obj["count"]    = cnt;
          obj["seuil"]    = slvSeuil;
          obj["active"]   = active;
        }
      }
    }
    
    doc["total"] = totalPersonnes;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // ─── GET /api/slaves/configured ───────────────────────────────────────────
  // Retourne la liste des slaves configurés manuellement dans l'interface
  server.on("/api/slaves/configured", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<2048> doc;
    JsonArray array = doc.to<JsonArray>();
    for (const auto& s : s_configuredSlaves) {
      JsonObject obj = array.createNestedObject();
      obj["mac"]          = s.mac;
      obj["moduleId"]     = s.moduleId;
      obj["wifiSSID"]     = s.wifiSSID;
    }
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // ─── POST /api/slaves/add ──────────────────────────────────────────────────
  server.on("/api/slaves/add", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<256> doc;
      deserializeJson(doc, data, len);
      
      if (!doc.containsKey("mac")) {
        request->send(400, "application/json", "{\"error\":\"Champ mac manquant\"}");
        return;
      }
      
      String mac      = doc["mac"].as<String>();
      String defaultId = "Slave-" + String(s_configuredSlaves.size() + 1);
      String slvId     = doc["moduleId"].as<String>();
      if (slvId.isEmpty()) slvId = defaultId;
      
      String wSSID    = doc["wifiSSID"].as<String>();
      if (wSSID.isEmpty()) wSSID = apSSID;
      String wPass    = doc["wifiPassword"].as<String>();
      if (wPass.isEmpty()) wPass = apPassword;
      
      // Vérifier qu'il n'existe pas déjà
      for (const auto& s : s_configuredSlaves) {
        if (s.mac.equalsIgnoreCase(mac)) {
          request->send(200, "application/json", "{\"status\":\"already_exists\"}");
          return;
        }
      }
      
      ConfiguredSlave cs;
      cs.mac          = mac;
      cs.moduleId     = slvId;
      cs.wifiSSID     = wSSID;
      cs.wifiPassword = wPass;
      s_configuredSlaves.push_back(cs);
      
      // Envoyer le paquet de pairing via ESP-NOW
      uint8_t targetMac[6];
      int macVal[6];
      if (sscanf(mac.c_str(), "%x:%x:%x:%x:%x:%x", 
          &macVal[0], &macVal[1], &macVal[2], &macVal[3], &macVal[4], &macVal[5]) == 6) {
        for (int i = 0; i < 6; i++) {
          targetMac[i] = (uint8_t)macVal[i];
        }
        espnow_sendPairing(targetMac, slvId.c_str(), wSSID.c_str(), wPass.c_str(), WiFi.macAddress().c_str());
      }
      
      request->send(200, "application/json", "{\"status\":\"ok\"}");
      Serial.printf("[Master] Slave configuré ajouté et pairing envoyé : %s (%s)\n", mac.c_str(), slvId.c_str());
    }
  );

  // ─── POST /api/slaves/delete ───────────────────────────────────────────────
  server.on("/api/slaves/delete", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<128> doc;
      deserializeJson(doc, data, len);
      
      if (!doc.containsKey("mac")) {
        request->send(400, "application/json", "{\"error\":\"Champ mac manquant\"}");
        return;
      }
      
      String mac = doc["mac"].as<String>();
      
      // Supprimer des slaves configurés
      for (auto it = s_configuredSlaves.begin(); it != s_configuredSlaves.end(); ++it) {
        if (it->mac.equalsIgnoreCase(mac)) {
          s_configuredSlaves.erase(it);
          request->send(200, "application/json", "{\"status\":\"ok\"}");
          return;
        }
      }
      
      // Supprimer aussi des slaves auto-découverts si présent
      for (auto it = s_registeredSlaves.begin(); it != s_registeredSlaves.end(); ++it) {
        if (it->mac.equalsIgnoreCase(mac)) {
          s_registeredSlaves.erase(it);
          request->send(200, "application/json", "{\"status\":\"ok\"}");
          return;
        }
      }
      
      request->send(404, "application/json", "{\"error\":\"Slave non trouvé\"}");
    }
  );

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
    StaticJsonDocument<512> doc;
    doc["role"]      = moduleRole;
    doc["moduleId"]  = moduleId;
    doc["masterMAC"] = masterMAC;
    if (moduleRole == "slave") {
      doc["wifiSSID"]     = staSSID;
      doc["wifiPassword"] = staPassword;
    }
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // ─── POST /api/config/module ──────────────────────────────────────────────
  server.on("/api/config/module", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<512> doc;
      deserializeJson(doc, data, len);
      // macSlave correspond au champ "MAC autre module" (mac2) envoyé par le formulaire
      if (doc.containsKey("macSlave"))   masterMAC = doc["macSlave"].as<String>();
      else if (doc.containsKey("masterMAC")) masterMAC = doc["masterMAC"].as<String>();
      else if (doc.containsKey("macMaster")) masterMAC = doc["macMaster"].as<String>();
      
      // Pour le rôle esclave, on enregistre aussi les identifiants pour se connecter au Master AP
      if (doc.containsKey("wifiSSID"))     staSSID = doc["wifiSSID"].as<String>();
      if (doc.containsKey("wifiPassword")) staPassword = doc["wifiPassword"].as<String>();
      
      // Détermination automatique du rôle
      if (masterMAC != "" && masterMAC != "00:00:00:00:00:00" && staSSID != "") {
        moduleRole = "slave";
      } else {
        moduleRole = "master";
        masterMAC  = "00:00:00:00:00:00";
        staSSID    = "";
        staPassword = "";
      }
      
      // moduleId personnalisé
      if (doc.containsKey("moduleId")) {
        moduleId = doc["moduleId"].as<String>();
      } else {
        // Auto-générer si pas fourni
        moduleId = (moduleRole == "master") ? "Master" : "Slave";
      }
      
      isMasterConfigured = true;
      
      storage_saveConfig();
      request->send(200, "application/json", "{\"status\":\"ok\"}");
      
      // Redémarrage différé
      requestReboot = true;
      rebootTimer = millis();
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
      
      staSSID = "";
      staPassword = "";
      WiFi.disconnect(true);
      
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
  server.on("/api/config/sensor", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<256> doc;
      deserializeJson(doc, data, len);
      if (!doc.containsKey("seuil")) {
        request->send(400, "application/json", "{\"error\":\"Champ seuil manquant\"}");
        return;
      }
      int newSeuil = doc["seuil"].as<int>();
      if (newSeuil < SEUIL_MIN || newSeuil > SEUIL_MAX) {
        request->send(400, "application/json", "{\"error\":\"Valeur hors limites (10-100 cm)\"}");
        return;
      }

      if (doc.containsKey("mac")) {
        String targetMacStr = doc["mac"].as<String>();
        if (!targetMacStr.equalsIgnoreCase(WiFi.macAddress())) {
          uint8_t targetMac[6];
          int macVal[6];
          if (sscanf(targetMacStr.c_str(), "%x:%x:%x:%x:%x:%x", 
              &macVal[0], &macVal[1], &macVal[2], &macVal[3], &macVal[4], &macVal[5]) == 6) {
            for (int i = 0; i < 6; i++) {
              targetMac[i] = (uint8_t)macVal[i];
            }
            espnow_sendConfig(targetMac, newSeuil);
            request->send(200, "application/json", "{\"status\":\"ok\"}");
            Serial.printf("✓ Config de seuil (%d cm) envoyée à l'esclave %s\n", newSeuil, targetMacStr.c_str());
            return;
          } else {
            request->send(400, "application/json", "{\"error\":\"MAC esclave invalide\"}");
            return;
          }
        }
      }

      seuil = newSeuil;
      storage_saveConfig();
      Serial.printf("✓ Seuil de détection local mis à jour : %d cm\n", seuil);
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

  // ─── POST /api/reset/count ────────────────────────────────────────────────
  server.on("/api/reset/count", HTTP_POST, [](AsyncWebServerRequest* request) {
    compteur           = 0;
    totalPersonnes     = 0;
    personnesActuelles = 0;
    storage_markDirty();
    webserver_broadcastCount(0);
    // Also reset all registered slaves counts (display only)
    for (auto& s : s_registeredSlaves) {
      s.count = 0;
    }
    Serial.println("[API] Reset comptage demandé depuis le dashboard.");
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // ─── POST /api/reset/module ───────────────────────────────────────────────
  server.on("/api/reset/module", HTTP_POST, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Redemarrage en cours\"}");
    requestReboot = true;
    rebootTimer   = millis();
    Serial.println("[API] Redémarrage module demandé depuis le dashboard.");
  });

  // ─── POST /api/reset/format ───────────────────────────────────────────────
  server.on("/api/reset/format", HTTP_POST, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Formatage et redemarrage en cours\"}");
    shouldFormat  = true;
    requestReboot = true;
    rebootTimer   = millis();
    Serial.println("[API] Formatage module demandé.");
  });

  // ─── POST /api/config/license ──────────────────────────────────────────────
  server.on("/api/config/license", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<512> doc;
      deserializeJson(doc, data, len);
      
      if (doc.containsKey("licence")) licenseCode = doc["licence"].as<String>();
      else if (doc.containsKey("licenseCode")) licenseCode = doc["licenseCode"].as<String>();
      
      if (doc.containsKey("date")) licenseDate = doc["date"].as<String>();
      if (doc.containsKey("duration")) licenseDuration = doc["duration"].as<int>();
      if (doc.containsKey("expiry")) licenseExpiry = doc["expiry"].as<String>();
      
      storage_saveConfig();
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }
  );

  // ─── POST /api/module/reset ───────────────────────────────────────────────
  server.on("/api/module/reset", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<256> doc;
      deserializeJson(doc, data, len);
      if (doc.containsKey("mac")) {
        String targetMac = doc["mac"].as<String>();
        
        // Reset local Master module
        if (targetMac.equalsIgnoreCase(WiFi.macAddress())) {
          if (moduleRole == "master") {
            totalPersonnes -= compteur;
            if (totalPersonnes < 0) totalPersonnes = 0;
            personnesActuelles = totalPersonnes;
          }
          compteur = 0;
          storage_markDirty();
          triggerImmediateDisplayUpdate();
          int valToShow = (moduleRole == "master") ? totalPersonnes : compteur;
          webserver_broadcastCount(valToShow);
          request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Compteur Master reset\"}");
          return;
        }
        
        // Reset Slave module (ESP-NOW)
        uint8_t macBytes[6];
        int macVal[6];
        if (sscanf(targetMac.c_str(), "%x:%x:%x:%x:%x:%x", 
            &macVal[0], &macVal[1], &macVal[2], &macVal[3], &macVal[4], &macVal[5]) == 6) {
          for (int i = 0; i < 6; i++) macBytes[i] = (uint8_t)macVal[i];
          espnow_sendReset(macBytes);
        }
        
        // Reset Slave count locally on Master records
        for (auto& s : s_registeredSlaves) {
          if (s.mac.equalsIgnoreCase(targetMac)) {
            int diff = -s.count;
            s.count = 0;
            s.lastSeen = millis();
            totalPersonnes += diff;
            if (totalPersonnes < 0) totalPersonnes = 0;
            personnesActuelles = totalPersonnes;
            break;
          }
        }
        
        triggerImmediateDisplayUpdate();
        webserver_broadcastCount(totalPersonnes);
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        request->send(400, "application/json", "{\"error\":\"MAC manquante\"}");
      }
    }
  );

  // ─── POST /api/module/format ──────────────────────────────────────────────
  server.on("/api/module/format", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<256> doc;
      deserializeJson(doc, data, len);
      if (doc.containsKey("mac")) {
        String targetMac = doc["mac"].as<String>();
        
        // Format local Master module
        if (targetMac.equalsIgnoreCase(WiFi.macAddress())) {
          request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Formatage Master lancé\"}");
          shouldFormat  = true;
          requestReboot = true;
          rebootTimer   = millis();
          return;
        }
        
        // Format Slave module (ESP-NOW)
        uint8_t macBytes[6];
        int macVal[6];
        if (sscanf(targetMac.c_str(), "%x:%x:%x:%x:%x:%x", 
            &macVal[0], &macVal[1], &macVal[2], &macVal[3], &macVal[4], &macVal[5]) == 6) {
          for (int i = 0; i < 6; i++) macBytes[i] = (uint8_t)macVal[i];
          espnow_sendFormat(macBytes);
        }
        
        // Remove from local Master lists
        for (auto it = s_registeredSlaves.begin(); it != s_registeredSlaves.end(); ++it) {
          if (it->mac.equalsIgnoreCase(targetMac)) {
            s_registeredSlaves.erase(it);
            break;
          }
        }
        
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        request->send(400, "application/json", "{\"error\":\"MAC manquante\"}");
      }
    }
  );

  server.begin();
  Serial.println("Serveur démarré !");
}

void webserver_broadcastCount(int val) {
  // Broadcast as JSON so the front-end can sync total
  String json = "{\"total\":" + String(val) + "}";
  ws.textAll(json);
}

String webserver_getMqttPayloadJson() {
  DynamicJsonDocument doc(4096);
  
  unsigned long now = millis();

  // Métadonnées globales
  doc["timestamp"]    = now;
  doc["battery"]      = 100;          // placeholder ADC
  doc["licence"]      = licenseCode;
  doc["total"]        = totalPersonnes;
  doc["current"]      = personnesActuelles;
  doc["ip"]           = WiFi.localIP().toString();
  doc["connected"]    = (WiFi.status() == WL_CONNECTED);

  // Sous-objet master
  JsonObject masterObj = doc.createNestedObject("master");
  masterObj["moduleId"]  = moduleId;
  masterObj["role"]      = moduleRole;
  masterObj["mac"]       = WiFi.macAddress();
  masterObj["count"]     = compteur;
  masterObj["seuil"]     = seuil;
  masterObj["battery"]   = 100;
  masterObj["active"]    = true;
  masterObj["licence"]   = licenseCode;

  // Tableau détaillé de tous les modules
  JsonArray modules = doc.createNestedArray("modules");

  // Master entry
  JsonObject masterMod = modules.createNestedObject();
  masterMod["moduleId"] = moduleId;
  masterMod["role"]     = "master";
  masterMod["mac"]      = WiFi.macAddress();
  masterMod["count"]    = compteur;
  masterMod["seuil"]    = seuil;
  masterMod["battery"]  = 100;
  masterMod["active"]   = true;

  int numModules = 1;
  int slavesTotal = 0;

  // Slaves HTTP
  for (const auto& s : s_registeredSlaves) {
    bool isActive = (now - s.lastSeen < 30000);
    JsonObject obj = modules.createNestedObject();
    obj["moduleId"] = s.moduleId;
    obj["role"]     = "slave";
    obj["mac"]      = s.mac;
    obj["ip"]       = s.ip;
    obj["count"]    = s.count;
    obj["seuil"]    = s.seuil;
    obj["battery"]  = 100;   // slaves n'ont pas encore ADC
    obj["active"]   = isActive;
    obj["source"]   = "wifi";
    if (isActive) { numModules++; slavesTotal += s.count; }
  }

  // Slaves ESP-NOW
  int espNowCount = espnow_getSlaveCount();
  for (int i = 0; i < espNowCount; i++) {
    char macBuf[18], idBuf[20];
    int  cnt;
    bool active;
    int  slvSeuil;
    if (espnow_getSlaveInfo(i, macBuf, idBuf, &cnt, &active, &slvSeuil)) {
      bool alreadyIn = false;
      for (const auto& s : s_registeredSlaves) {
        if (s.mac.equalsIgnoreCase(macBuf)) { alreadyIn = true; break; }
      }
      if (!alreadyIn) {
        JsonObject obj = modules.createNestedObject();
        obj["moduleId"] = String(idBuf[0] ? idBuf : ("Slave-" + String(i + 1)).c_str());
        obj["role"]     = "slave";
        obj["mac"]      = String(macBuf);
        obj["count"]    = cnt;
        obj["seuil"]    = slvSeuil;
        obj["battery"]  = 100;
        obj["active"]   = active;
        obj["source"]   = "espnow";
        if (active) { numModules++; slavesTotal += cnt; }
      }
    }
  }

  doc["modulesCount"]  = numModules;
  doc["masterCount"]   = compteur;
  doc["slavesTotal"]   = slavesTotal;

  String output;
  serializeJson(doc, output);
  return output;
}

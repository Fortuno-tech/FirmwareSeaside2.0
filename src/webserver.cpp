#include "webserver.h"
#include "config.h"
#include "wifi_ap.h"

#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#endif

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
    doc["espnow"] = "inactive";
    doc["ip"]     = WiFi.softAPIP().toString();
    doc["ssid"]   = apSSID;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<200> doc;
    doc["ssid"]     = apSSID;
    doc["password"] = apPassword;
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/api/config", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<200> doc;
      deserializeJson(doc, data, len);

      String newSSID     = doc["ssid"]     | apSSID;
      String newPassword = doc["password"] | apPassword;

      if (newSSID.length() < 1 || newPassword.length() < 8) {
        request->send(400, "application/json",
          "{\"error\":\"SSID vide ou MDP trop court (min 8)\"}");
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
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

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<256> doc;
    doc["total"]  = totalPersonnes;
    doc["current"] = personnesActuelles;
    doc["role"]   = moduleRole;
    doc["ip"]     = WiFi.softAPIP().toString();
    doc["mac"]    = WiFi.macAddress();
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  server.on("/api/config/module", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<256> doc;
      deserializeJson(doc, data, len);
      if(doc.containsKey("role")) moduleRole = doc["role"].as<String>();
      if(doc.containsKey("masterMAC")) masterMAC = doc["masterMAC"].as<String>();
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }
  );

  server.on("/api/config/ap", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      StaticJsonDocument<256> doc;
      deserializeJson(doc, data, len);
      String newSSID = doc["ssid"] | apSSID;
      String newPass = doc["password"] | apPassword;
      modifierAP(newSSID, newPass);
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    }
  );

  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "application/json", "{\"error\":\"Not found\"}");
  });

  server.begin();
}

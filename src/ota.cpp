#include "ota.h"
#include <ElegantOTA.h>
#include <ESPAsyncWebServer.h>

extern AsyncWebServer server;

void onOTAStart() {
  Serial.println("OTA: Mise à  jour démarré...");
}

void onOTAProgress(size_t current, size_t final) {
  // Optionnel : Log de progression
}

void onOTAEnd(bool success) {
  if (success) {
    Serial.println("OTA: Mise à  jour réussie !");
  } else {
    Serial.println("OTA: Echec de la mise à  jour.");
  }
}

void setupOTA() {
  ElegantOTA.begin(&server);    // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
  
  Serial.println("ElegantOTA pret sur http://192.168.4.1/update");
}

void handleOTA() {
  ElegantOTA.loop();
}

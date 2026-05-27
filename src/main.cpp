#include <Arduino.h>
#include <LittleFS.h>
#include "config.h"
#include "wifi_ap.h"
#include "webserver.h"

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== Seaside 2.0 - Boot ===");

  if (!LittleFS.begin()) {
    Serial.println("Erreur LittleFS !");
  } else {
    Serial.println("LittleFS OK");
  }

  setupAP();
  setupServer();
}

void loop() {
}
#include <Arduino.h>
#include <LittleFS.h>

#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#endif

#include <ESPAsyncWebServer.h>
#include "config.h"
#include "wifi_ap.h"
#include "webserver.h"

#ifdef ESP32
  #include "espnow.h"
#else
  #include "udp.h"
#endif

// Rôle du module : "master" ou "slave"
#define ROLE "master"

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== Seaside 2.0 - Boot ===");

  // LittleFS
  if (!LittleFS.begin()) {
    Serial.println("Erreur LittleFS !");
  } else {
    Serial.println("LittleFS OK");
  }

  // WiFi AP
  setupAP();

  // Serveur HTTP
  setupServer();

  // Communication
  #ifdef ESP32
    if (String(ROLE) == "master") {
      setupESPNOW_Master();
    } else {
      setupESPNOW_Slave();
    }
  #else
    if (String(ROLE) == "master") {
      setupUDP_Master();
    } else {
      setupUDP_Slave();
    }
  #endif

  Serial.print("Rôle : ");
  Serial.println(ROLE);
}

void loop() {
  #ifndef ESP32
    // ESP8266 Master écoute UDP en permanence
    if (String(ROLE) == "master") {
      udp_receive();
    }
  #endif
}
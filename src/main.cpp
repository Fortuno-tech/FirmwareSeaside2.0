#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "config.h"
#include "wifi_ap.h"
#include "webserver.h"
#include "espnow.h"
#include "ota.h"
#include "mqtt.h"

#define LED_PIN 2

// WiFi Internet pour MQTT
const char* STA_SSID = "TonWiFi";
const char* STA_PASS = "TonMotDePasse";

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  delay(1000);
  Serial.println("=== Seaside 2.0 - Boot ===");

  if (!LittleFS.begin()) {
    Serial.println("Erreur LittleFS !");
  } else {
    Serial.println("LittleFS OK");
  }

  setupAP();
  setupServer();
  setupOTA();

  if (moduleRole == "master") {
    setupESPNOW_Master();
    setupMQTT(STA_SSID, STA_PASS);
  } else {
    setupESPNOW_Slave();
  }

  Serial.print("Role : ");
  Serial.println(moduleRole);
}

void loop() {
  handleOTA();
  handleMQTT();

  if (moduleRole == "slave") {
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 2000) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      lastBlink = millis();
      Serial.println("Slave Blink...");

      if (masterMAC != "00:00:00:00:00:00") {
        uint8_t macBytes[6];
        int values[6];
        if (sscanf(masterMAC.c_str(), "%x:%x:%x:%x:%x:%x",
            &values[0], &values[1], &values[2],
            &values[3], &values[4], &values[5]) == 6) {
          for (int i = 0; i < 6; i++) macBytes[i] = (uint8_t)values[i];
          espnow_sendData(macBytes, personnesActuelles);
        }
      }
    }
  } else {
    digitalWrite(LED_PIN, HIGH);

    // Master publie le comptage toutes les 5s
    static unsigned long lastPublish = 0;
    if (millis() - lastPublish > 5000) {
      lastPublish = millis();
      mqttPublishCount(totalPersonnes, personnesActuelles);
    }
  }
}
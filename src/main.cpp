#include <Arduino.h>

#include "config.h"
#include "display.h"
#include "ultrasonic.h"
#include "buzzer_led.h"
#include "buttons.h"
#include "storage.h"
#include "wifi_ap.h"
#include "webserver.h"
#include "espnow.h"
#include "mqtt.h"
#include "ota.h"

// â”€â”€â”€ Ã‰tat global de l'application â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
int                  compteur        = 0;
static bool          objetDetecte    = false;
static bool          displayEnabled  = true;
static unsigned long lastActivityTime = 0;
static unsigned long lastLoopTime    = 0;

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Callbacks boutons
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

static void onButtonPlus() {
  compteur++;
  lastActivityTime = millis();
  storage_markDirty();
  Serial.print("â†’ Bouton + | Compteur = ");
  Serial.println(compteur);
}

static void onButtonMinus() {
  if (compteur > 0) compteur--;
  lastActivityTime = millis();
  storage_markDirty();
  Serial.print("â†’ Bouton - | Compteur = ");
  Serial.println(compteur);
}

static void onButtonReset() {
  compteur = 0;
  lastActivityTime = millis();
  storage_markDirty();
  buzzerLed_doubleBeep();
  Serial.println("!!! BOUTON RESET - COMPTEUR REMIS Ã€ ZÃ‰RO !!!");
}

static void onWake() {
  displayEnabled   = true;
  lastActivityTime = millis();
}

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// Détection ultrason
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

static void handleUltrasonic() {
  float distance = ultrasonic_readDistance();

  if (distance > 0 && distance < SEUIL) {
    if (!objetDetecte) {
      compteur++;
      objetDetecte      = true;
      lastActivityTime  = millis();
      displayEnabled    = true;
      storage_markDirty();
      buzzerLed_trigger();
      Serial.print("â†’ Obstacle détecté ! Compteur = ");
      Serial.println(compteur);
    }
  } else {
    objetDetecte = false;
  }
}

// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
// setup / loop
// â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void setup() {
  Serial.begin(115200);

  // Shift register
  pinMode(DATA_PIN,  OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);

  display_init();
  ultrasonic_init();
  buzzerLed_init();
  buttons_init();
  buttons_setCallbacks(onButtonPlus, onButtonMinus, onButtonReset, onWake);

  if (storage_init()) {
    compteur = storage_load();
    storage_loadConfig();
  }

  // WiFi et Services
  setupAP();
  setupServer();
  setupOTA();

  // Configuration selon le rôle
  if (moduleRole == "master") {
    setupESPNOW_Master();
    if (staSSID != "") {
      setupMQTT(staSSID.c_str(), staPassword.c_str());
    }
  } else if (moduleRole == "slave") {
    setupESPNOW_Slave();
  }

  lastActivityTime = millis();
  lastLoopTime     = millis();

  Serial.println("=== Système Seaside 2.0 Prêt ===");
  Serial.print("Rôle actuel : ");
  Serial.println(moduleRole);
}

void loop() {
  unsigned long now = millis();

  // Initialisation WiFi/MQTT différée demandée par le serveur web
  if (mqttTriggerSetup) {
    mqttTriggerSetup = false;
    if (staSSID != "") {
      setupMQTT(staSSID.c_str(), staPassword.c_str());
    }
  }

  // Modules de base
  buzzerLed_update();
  buttons_update(displayEnabled);
  handleUltrasonic();
  storage_update(compteur);
  handleOTA();

  // Logique Master
  if (moduleRole == "master") {
    handleMQTT();
    static unsigned long lastPub = 0;
    if (now - lastPub > 5000) {
      mqttPublishCount(totalPersonnes, personnesActuelles);
      lastPub = now;
    }
  } 
  // Logique Slave
  else if (moduleRole == "slave") {
    static int lastSent = -1;
    if (compteur != lastSent && masterMAC != "00:00:00:00:00:00") {
      uint8_t mMac[6];
      if (sscanf(masterMAC.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
          &mMac[0], &mMac[1], &mMac[2], &mMac[3], &mMac[4], &mMac[5]) == 6) {
        espnow_addSlave(mMac); // Utilise addSlave pour enregistrer le Master comme peer
        espnow_sendData(mMac, compteur);
        lastSent = compteur;
      }
    }
  }

  // Veille et Affichage
  displayEnabled = (now - lastActivityTime < INACTIVITY_TIMEOUT);
  if (now - lastLoopTime >= LOOP_INTERVAL) {
    // Si Master, on peut choisir d'afficher totalPersonnes au lieu de compteur local
    int valToShow = (moduleRole == "master") ? totalPersonnes : compteur;
    display_showNumber(valToShow, displayEnabled);
    lastLoopTime = now;
  }
}


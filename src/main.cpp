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

//Etat app
int                  compteur        = 0;
static bool          objetDetecte    = false;
static bool          displayEnabled  = true;
static unsigned long lastActivityTime = 0;
static unsigned long lastLoopTime    = 0;

//buttons
static void onButtonPlus() {
  compteur++;
  if (moduleRole == "master") {
    totalPersonnes = compteur;
    personnesActuelles = compteur;
  }
  lastActivityTime = millis();
  storage_markDirty();
  Serial.print("→ Bouton + | Compteur = ");
  Serial.println(compteur);

  int valToShow = (moduleRole == "master") ? totalPersonnes : compteur;
  webserver_broadcastCount(valToShow);
}

static void onButtonMinus() {
  if (compteur > 0) compteur--;
  if (moduleRole == "master") {
    totalPersonnes = compteur;
    personnesActuelles = compteur;
  }
  lastActivityTime = millis();
  storage_markDirty();
  Serial.print("→ Bouton - | Compteur = ");
  Serial.println(compteur);

  int valToShow = (moduleRole == "master") ? totalPersonnes : compteur;
  webserver_broadcastCount(valToShow);
}

static void onButtonReset() {
  compteur = 0;
  if (moduleRole == "master") {
    totalPersonnes = 0;
    personnesActuelles = 0;
  }
  lastActivityTime = millis();
  storage_markDirty();
  buzzerLed_doubleBeep();
  Serial.println("!!! BOUTON RESET - COMPTEUR REMIS À ZÉRO !!!");

  int valToShow = (moduleRole == "master") ? totalPersonnes : compteur;
  webserver_broadcastCount(valToShow);
}

static void onWake() {
  displayEnabled   = true;
  lastActivityTime = millis();
}

// ─────────────────────────────────────────────────────────────────────────────
// Détection ultrason
// ─────────────────────────────────────────────────────────────────────────────

static void handleUltrasonic() {
  unsigned long now = millis();
  static unsigned long lastSensorRead = 0;
  static unsigned long lastDetectionTime = 0;

  // Interroger le capteur toutes les 100ms max (pour éviter les échos parasites)
  if (now - lastSensorRead < 100) {
    return;
  }
  lastSensorRead = now;

  float distance = ultrasonic_readDistance();

  if (distance > 0 && distance < SEUIL) {
    // Cooldown de 1.5s entre deux incrémentations (temps de passage d'une personne)
    if (!objetDetecte && (now - lastDetectionTime >= 1500)) {
      compteur++;
      objetDetecte      = true;
      lastDetectionTime = now;
      lastActivityTime  = now;
      displayEnabled    = true;
      storage_markDirty();
      buzzerLed_trigger();
      Serial.print("→ Obstacle détecté ! Compteur = ");
      Serial.println(compteur);

      if (moduleRole == "master") {
        totalPersonnes     = compteur;
        personnesActuelles = compteur;
      }

      int valToShow = (moduleRole == "master") ? totalPersonnes : compteur;
      webserver_broadcastCount(valToShow);
    }
  } else {
    // Hystérésis temporelle : l'obstacle doit être absent pendant 500ms d'affilée pour libérer le capteur
    static unsigned long lastObstacleTime = 0;
    if (objetDetecte) {
      if (lastObstacleTime == 0) {
        lastObstacleTime = now;
      } else if (now - lastObstacleTime > 500) {
        objetDetecte = false;
        lastObstacleTime = 0;
      }
    } else {
      lastObstacleTime = 0;
    }
  }
}

// setup / loop

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


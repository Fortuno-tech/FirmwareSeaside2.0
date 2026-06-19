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

// FIX Bug 1 : initialiser lastDetectionTime à une valeur
// qui garantit que le cooldown est "déjà écoulé" au démarrage,
// mais on attend quand même que le capteur fasse une lecture valide.
static unsigned long lastDetectionTime  = 0;
static bool          bootGuardDone      = false;  // protège la 1ère lecture

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

// ─────────────────────────────────────────────────────────────────────────────────
// Détection ultrason
// ─────────────────────────────────────────────────────────────────────────────────

static void handleUltrasonic() {
  unsigned long now = millis();
  static unsigned long lastSensorRead   = 0;
  // FIX Bug 2 : lastObstacleTime déclarée ICI (scope fonction) et non dans le else
  static unsigned long lastObstacleTime = 0;

  // Interroger le capteur toutes les 100 ms (anti-échos parasites)
  if (now - lastSensorRead < 100) return;
  lastSensorRead = now;

  float distance = ultrasonic_readDistance();

  // FIX Bug 3 : timeout capteur (distance == 0.0) = pas d'écho reçu
  // On traite ça comme "rien devant" (voie libre) plutôt que de l'ignorer.
  // Cela évite de bloquer indefiniment objetDetecte=true si le capteur perd l'écho.
  bool voibreLibre = (distance <= 0.0f || distance >= (float)seuil);

  // FIX Bug 1 : protection démarrage
  // On laisse passer 2 lectures valides avant d'activer la détection,
  // pour ne pas comptabiliser un objet déjà présent au boot.
  if (!bootGuardDone) {
    static uint8_t bootReadCount = 0;
    bootReadCount++;
    if (bootReadCount >= 3) {
      bootGuardDone    = true;
      lastDetectionTime = now;   // cooldown réinitialisé proprement
      Serial.printf("[SENSOR] Boot guard OK – seuil actif : %d cm\n", seuil);
    } else {
      Serial.printf("[SENSOR] Boot guard %d/3 – dist=%.1f cm\n", bootReadCount, distance);
    }
    return;
  }

  // ─ Log de débogage périodique (toutes les 500 ms environ) ───────────────
  static unsigned long lastLogTime = 0;
  if (now - lastLogTime >= 500) {
    lastLogTime = now;
    Serial.printf("[SENSOR] dist=%.1f cm | seuil=%d cm | detecete=%d | cooldown=%lu ms restant\n",
      distance, seuil, (int)objetDetecte,
      (now - lastDetectionTime < 1500) ? (1500 - (now - lastDetectionTime)) : 0);
  }

  if (!voibreLibre) {
    // ── Objet dans la zone de détection ──────────────────────────────

    // FIX Bug 5 : réinitialiser lastObstacleTime dès que l'objet est détecté
    // (sinon le timer d'hysterésis continue de tourner entre deux lectures)
    lastObstacleTime = 0;

    if (!objetDetecte) {
      if (now - lastDetectionTime >= 1500) {
        // ── PASSAGE VALIDÉ ────────────────────────────────────────────
        compteur++;
        objetDetecte      = true;
        lastDetectionTime = now;
        lastActivityTime  = now;
        displayEnabled    = true;
        storage_markDirty();
        buzzerLed_trigger();

        Serial.printf("\n>>> PASSAGE DÉTECTÉ ! dist=%.1f cm | seuil=%d cm | compteur=%d\n\n",
          distance, seuil, compteur);

        if (moduleRole == "master") {
          totalPersonnes     = compteur;
          personnesActuelles = compteur;
        }
        int valToShow = (moduleRole == "master") ? totalPersonnes : compteur;
        webserver_broadcastCount(valToShow);

      } else {
        // FIX Bug 4 : log explicite quand le cooldown bloque
        Serial.printf("[SENSOR] Passage ignoré (cooldown %lu ms restant)\n",
          1500 - (now - lastDetectionTime));
      }
    }

  } else {
    // ── Voie libre (ou timeout capteur) ──────────────────────────────
    if (objetDetecte) {
      if (lastObstacleTime == 0) {
        // Début de la fenêtre d'hysterésis
        lastObstacleTime = now;
      } else if (now - lastObstacleTime > 500) {
        // FIX Bug 2 : liburation propre avec log
        Serial.printf("[SENSOR] Voie libérée après hysterésis 500 ms\n");
        objetDetecte     = false;
        lastObstacleTime = 0;
      }
    } else {
      // Pas d'objet, pas d'hysterésis en cours : reset du timer
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


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
#include "battery.h"

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

void triggerImmediateDisplayUpdate() {
  displayEnabled   = true;
  lastActivityTime = millis();
  int valToShow    = (moduleRole == "master") ? totalPersonnes : compteur;
  display_showNumber(valToShow, displayEnabled);
  lastLoopTime     = millis();
}

//buttons
static void onButtonPlus() {
  compteur++;
  if (moduleRole == "master") {
    totalPersonnes++;
    personnesActuelles++;
  }
  triggerImmediateDisplayUpdate();
  storage_markDirty();
  Serial.print("→ Bouton + | Compteur = ");
  Serial.println(compteur);

  int valToShow = (moduleRole == "master") ? totalPersonnes : compteur;
  webserver_broadcastCount(valToShow);
}

static void onButtonMinus() {
  if (compteur > 0) {
    compteur--;
    if (moduleRole == "master") {
      if (totalPersonnes > 0) totalPersonnes--;
      if (personnesActuelles > 0) personnesActuelles--;
    }
  }
  triggerImmediateDisplayUpdate();
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
    espnow_resetSlaves();
  }
  triggerImmediateDisplayUpdate();
  storage_markDirty();
  buzzerLed_doubleBeep();
  Serial.println("!!! BOUTON RESET - COMPTEUR REMIS À ZÉRO !!!");

  int valToShow = (moduleRole == "master") ? totalPersonnes : compteur;
  webserver_broadcastCount(valToShow);
}

static void onWake() {
  triggerImmediateDisplayUpdate();
}

void mqttCommandIncrementer() { onButtonPlus(); }
void mqttCommandDecrementer() { onButtonMinus(); }
void mqttCommandResetCompteur() { onButtonReset(); }

// ─────────────────────────────────────────────────────────────────────────────────
// Détection ultrason
// ─────────────────────────────────────────────────────────────────────────────────

static void handleUltrasonic() {
  unsigned long now = millis();
  static unsigned long lastSensorRead   = 0;
  // FIX Bug 2 : lastObstacleTime déclarée ICI (scope fonction) et non dans le else
  static unsigned long lastObstacleTime = 0;
  static int lastSeuilVal = seuil;

  // Interroger le capteur toutes les 100 ms (anti-échos parasites)
  if (now - lastSensorRead < 100) return;
  lastSensorRead = now;

  // Réinitialiser la garde si le seuil change
  if (seuil != lastSeuilVal) {
    lastSeuilVal = seuil;
    bootGuardDone = false;
    objetDetecte = false;
    Serial.printf("[SENSOR] Seuil modifié à %d cm. Réinitialisation de la garde...\n", seuil);
  }

  float distance = ultrasonic_readDistance();

  // FIX Bug 3 : timeout capteur (distance == 0.0) = pas d'écho reçu
  // On traite ça comme "rien devant" (voie libre) plutôt que de l'ignorer.
  // Cela évite de bloquer indefiniment objetDetecte=true si le capteur perd l'écho.
  bool voibreLibre = (distance <= 0.0f || distance >= (float)seuil);

  // FIX Bug 1 : protection démarrage / changement de seuil
  // On laisse passer 3 lectures valides avant d'activer la détection,
  // pour ne pas comptabiliser un objet déjà présent au boot ou après changement de seuil.
  if (!bootGuardDone) {
    static uint8_t bootReadCount = 0;
    // Si un objet est déjà présent au démarrage ou après changement, on le considère comme détecté
    if (distance > 0.0f && distance < (float)seuil) {
      objetDetecte = true;
    }
    bootReadCount++;
    if (bootReadCount >= 3) {
      bootGuardDone    = true;
      lastDetectionTime = now;   // cooldown réinitialisé proprement
      Serial.printf("[SENSOR] Garde OK – seuil actif : %d cm | objetDetecte initial = %d\n", seuil, (int)objetDetecte);
      bootReadCount = 0;         // Reset pour le prochain changement de seuil
    } else {
      Serial.printf("[SENSOR] Garde %d/3 – dist=%.1f cm\n", bootReadCount, distance);
    }
    return;
  }

  // ─ Log de débogage périodique (toutes les 500 ms environ) ───────────────
  static unsigned long lastLogTime = 0;
  if (now - lastLogTime >= 500) {
    lastLogTime = now;
    Serial.printf("[SENSOR] dist=%.1f cm | seuil=%d cm | detecete=%d | cooldown=%lu ms restant\n",
      distance, seuil, (int)objetDetecte,
      (now - lastDetectionTime < 800) ? (800 - (now - lastDetectionTime)) : 0);
  }

  if (!voibreLibre) {
    // ── Objet dans la zone de détection ──────────────────────────────

    // FIX Bug 5 : réinitialiser lastObstacleTime dès que l'objet est détecté
    // (sinon le timer d'hysterésis continue de tourner entre deux lectures)
    lastObstacleTime = 0;

    if (!objetDetecte) {
      if (now - lastDetectionTime >= 800) {
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
          // Le compteur local du Master doit fonctionner même sans licence.
          // La licence contrôle la synchronisation MQTT, pas l'affichage local.
          totalPersonnes++;
          personnesActuelles++;
          if (storage_isLicenseValid()) {
            mqttPublishEntry();
          } else {
            Serial.println("[License] Invalide - passage compté localement, publication MQTT ignorée");
          }
        }
        triggerImmediateDisplayUpdate();
        int valToShow = (moduleRole == "master") ? totalPersonnes : compteur;
        webserver_broadcastCount(valToShow);

      } else {
        // FIX Bug 4 : log explicite quand le cooldown bloque
        Serial.printf("[SENSOR] Passage ignoré (cooldown %lu ms restant)\n",
          800 - (now - lastDetectionTime));
      }
    }

  } else {
    // ── Voie libre (ou timeout capteur) ──────────────────────────────
    if (objetDetecte) {
      if (lastObstacleTime == 0) {
        // Début de la fenêtre d'hysterésis
        lastObstacleTime = now;
      } else if (now - lastObstacleTime > 300) {
        // FIX Bug 2 : libération propre avec log (hystérésis réduite à 300ms pour fluidité)
        Serial.printf("[SENSOR] Voie libérée après hysterésis 300 ms\n");
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
  battery_init();
  buttons_init();
  buttons_setCallbacks(onButtonPlus, onButtonMinus, onButtonReset, onWake);

  if (storage_init()) {
    compteur = storage_load();
    storage_loadConfig();
    if (moduleRole == "master") {
      totalPersonnes = compteur;
      personnesActuelles = compteur;
    }
  }

  // ─── Auto-Master : si aucun rôle n'a jamais été attribué, ce module devient Master ───
  if (!isMasterConfigured) {
    moduleRole = "master";
    moduleId   = "Master";
    isMasterConfigured = true;
    totalPersonnes = compteur;
    personnesActuelles = compteur;
    storage_saveConfig();
    Serial.println("=== Premier démarrage : rôle MASTER attribué automatiquement ===");
  }

  // Si moduleId est vide (migration depuis ancienne config), définir selon le rôle
  if (moduleId == "") {
    moduleId = (moduleRole == "master") ? "Master" : "Slave";
    storage_saveConfig();
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

  // Redémarrage différé
  if (requestReboot && (now - rebootTimer >= 2000)) {
    if (shouldFormat) {
      storage_format();
    }
    ESP.restart();
  }

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
  storage_update((moduleRole == "master") ? totalPersonnes : compteur);
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
    handleSlaveAnnounce();
    static int lastSent = -1;
    if (compteur != lastSent && masterMAC != "00:00:00:00:00:00") {
      int macVal[6];
      if (sscanf(masterMAC.c_str(), "%x:%x:%x:%x:%x:%x", 
          &macVal[0], &macVal[1], &macVal[2], &macVal[3], &macVal[4], &macVal[5]) == 6) {
        uint8_t mMac[6];
        for (int i = 0; i < 6; i++) {
          mMac[i] = (uint8_t)macVal[i];
        }
        espnow_addSlave(mMac); // Enregistre le Master comme peer si pas déjà fait
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


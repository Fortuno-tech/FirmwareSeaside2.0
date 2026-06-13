#include <Arduino.h>

#include "config.h"
#include "display.h"
#include "ultrasonic.h"
#include "buzzer_led.h"
#include "buttons.h"
#include "storage.h"

// ─── État global de l'application ────────────────────────────────────────────
static int           compteur        = 0;
static bool          objetDetecte    = false;
static bool          displayEnabled  = true;
static unsigned long lastActivityTime = 0;
static unsigned long lastLoopTime    = 0;

// ─────────────────────────────────────────────────────────────────────────────
// Callbacks boutons
// ─────────────────────────────────────────────────────────────────────────────

static void onButtonPlus() {
  compteur++;
  lastActivityTime = millis();
  storage_markDirty();
  Serial.print("→ Bouton + | Compteur = ");
  Serial.println(compteur);
}

static void onButtonMinus() {
  if (compteur > 0) compteur--;
  lastActivityTime = millis();
  storage_markDirty();
  Serial.print("→ Bouton - | Compteur = ");
  Serial.println(compteur);
}

static void onButtonReset() {
  compteur = 0;
  lastActivityTime = millis();
  storage_markDirty();
  buzzerLed_doubleBeep();
  Serial.println("!!! BOUTON RESET - COMPTEUR REMIS À ZÉRO !!!");
}

static void onWake() {
  displayEnabled   = true;
  lastActivityTime = millis();
}

// ─────────────────────────────────────────────────────────────────────────────
// Détection ultrason
// ─────────────────────────────────────────────────────────────────────────────

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
      Serial.print("→ Obstacle détecté ! Compteur = ");
      Serial.println(compteur);
    }
  } else {
    objetDetecte = false;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// setup / loop
// ─────────────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(9600);

  // Shift register (partagé par display)
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
  }

  lastActivityTime = millis();
  lastLoopTime     = millis();

  Serial.println("=== Système de comptage avec veille et mémoire ===");
  Serial.println("Veille automatique après 20 secondes d'inactivité");
  Serial.println("Boutons : + (pin25) | - (pin33) | RESET (pin27)");
  Serial.println("Le compteur est sauvegardé automatiquement");
}

// ─────────────────────────────────────────────────────────────────────────────

void loop() {
  unsigned long now = millis();

  // ── Modules non bloquants ────────────────────────────────────────────────
  buzzerLed_update();
  buttons_update(displayEnabled);
  handleUltrasonic();
  storage_update(compteur);

  // ── Gestion de la veille ─────────────────────────────────────────────────
  displayEnabled = (now - lastActivityTime < INACTIVITY_TIMEOUT);

  // ── Rafraîchissement afficheur ───────────────────────────────────────────
  if (now - lastLoopTime >= LOOP_INTERVAL) {
    display_showNumber(compteur, displayEnabled);
    lastLoopTime = now;
  }
}

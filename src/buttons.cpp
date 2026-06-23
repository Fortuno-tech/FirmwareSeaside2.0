#include "buttons.h"
#include "config.h"
#include "buzzer_led.h"
#include "storage.h"

// ─── Callbacks enregistrés ────────────────────────────────────────────────────
static ButtonCallback s_onPlus  = nullptr;
static ButtonCallback s_onMinus = nullptr;
static ButtonCallback s_onReset = nullptr;
static ButtonCallback s_onWake  = nullptr;

// ─── États anti-rebond et appui long ─────────────────────────────────────────
static bool          s_plusLastState = HIGH;
static unsigned long s_plusLastDebounceTime = 0;

static bool          s_minusLastState = HIGH;
static unsigned long s_minusLastDebounceTime = 0;

static bool          s_resetLastState = HIGH;
static unsigned long s_resetLastDebounceTime = 0;
static unsigned long s_resetPressStartTime = 0;
static bool          s_resetIsHeld = false;
static bool          s_formatTriggered = false;

// Temps global du dernier réveil pour la fenêtre de garde
static unsigned long s_lastWakeTime = 0;

// ─────────────────────────────────────────────────────────────────────────────

void buttons_setCallbacks(ButtonCallback onPlus,
                          ButtonCallback onMinus,
                          ButtonCallback onReset,
                          ButtonCallback onWake) {
  s_onPlus  = onPlus;
  s_onMinus = onMinus;
  s_onReset = onReset;
  s_onWake  = onWake;
}

// ─────────────────────────────────────────────────────────────────────────────

void buttons_init() {
  pinMode(BTN_PLUS,  INPUT_PULLUP);
  pinMode(BTN_MINUS, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);
}

// ─────────────────────────────────────────────────────────────────────────────

void buttons_update(bool displayEnabled) {
  unsigned long now = millis();

  // ─── BOUTON PLUS (+) ───────────────────────────────────────────────────────
  bool plusReading = digitalRead(BTN_PLUS);
  if (plusReading != s_plusLastState) {
    if (now - s_plusLastDebounceTime >= 50) {
      s_plusLastState = plusReading;
      s_plusLastDebounceTime = now;

      if (s_plusLastState == LOW) {
        if (!displayEnabled) {
          s_lastWakeTime = now;
          if (s_onWake) s_onWake();
          Serial.println("→ Réveil de l'afficheur (bouton +)");
        } else {
          if (now - s_lastWakeTime > WAKE_WINDOW) {
            if (s_onPlus) s_onPlus();
          }
        }
      }
    }
  } else {
    s_plusLastDebounceTime = now;
  }

  // ─── BOUTON MINUS (-) ──────────────────────────────────────────────────────
  bool minusReading = digitalRead(BTN_MINUS);
  if (minusReading != s_minusLastState) {
    if (now - s_minusLastDebounceTime >= 50) {
      s_minusLastState = minusReading;
      s_minusLastDebounceTime = now;

      if (s_minusLastState == LOW) {
        if (!displayEnabled) {
          s_lastWakeTime = now;
          if (s_onWake) s_onWake();
          Serial.println("→ Réveil de l'afficheur (bouton -)");
        } else {
          if (now - s_lastWakeTime > WAKE_WINDOW) {
            if (s_onMinus) s_onMinus();
          }
        }
      }
    }
  } else {
    s_minusLastDebounceTime = now;
  }

  // ─── BOUTON RESET / FORMAT ─────────────────────────────────────────────────
  bool resetReading = digitalRead(BTN_RESET);
  if (resetReading != s_resetLastState) {
    if (now - s_resetLastDebounceTime >= 50) {
      s_resetLastState = resetReading;
      s_resetLastDebounceTime = now;

      if (s_resetLastState == LOW) {
        if (!displayEnabled) {
          s_lastWakeTime = now;
          if (s_onWake) s_onWake();
          Serial.println("→ Réveil de l'afficheur (bouton RESET)");
        } else {
          if (now - s_lastWakeTime > WAKE_WINDOW) {
            s_resetPressStartTime = now;
            s_resetIsHeld = true;
            s_formatTriggered = false;
          }
        }
      } else {
        // Relâché
        if (s_resetIsHeld) {
          if (!s_formatTriggered) {
            if (s_onReset) s_onReset();
          }
          s_resetIsHeld = false;
        }
      }
    }
  } else {
    s_resetLastDebounceTime = now;
  }

  // Gestion du maintien pour formatage (3 secondes)
  if (s_resetIsHeld && !s_formatTriggered) {
    if (now - s_resetPressStartTime >= 3000) {
      s_formatTriggered = true;
      Serial.println("!!! BOUTON RESET MAINTENU : DEBUT FORMATAGE !!!");
      
      // Signal sonore/lumineux
      buzzerLed_trigger();
      delay(200);
      buzzerLed_trigger();
      
      storage_format();
      
      Serial.println("Formatage terminé. Redémarrage du système...");
      delay(500);
      ESP.restart();
    }
  }
}

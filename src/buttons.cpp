#include "buttons.h"
#include "config.h"

// ─── Callbacks enregistrés ────────────────────────────────────────────────────
static ButtonCallback s_onPlus  = nullptr;
static ButtonCallback s_onMinus = nullptr;
static ButtonCallback s_onReset = nullptr;
static ButtonCallback s_onWake  = nullptr;

// ─── État anti-rebond ─────────────────────────────────────────────────────────
static bool          s_prevPlus  = HIGH;
static bool          s_prevMinus = HIGH;
static bool          s_prevReset = HIGH;

static unsigned long s_debounceTimePlus  = 0;
static unsigned long s_debounceTimeMinus = 0;
static unsigned long s_debounceTimeReset = 0;

// ─── Fenêtres de réveil ───────────────────────────────────────────────────────
static unsigned long s_wakeTimePlus  = 0;
static unsigned long s_wakeTimeMinus = 0;
static unsigned long s_wakeTimeReset = 0;

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
// Macro interne pour factoriser la logique commune des 3 boutons
// ─────────────────────────────────────────────────────────────────────────────
static void handleButton(int          pin,
                         bool&        prevState,
                         unsigned long& debounceTime,
                         unsigned long& wakeTime,
                         ButtonCallback onAction,
                         bool           displayEnabled,
                         const char*    label)
{
  bool state = digitalRead(pin);
  unsigned long now = millis();

  if (prevState == HIGH && state == LOW) {          // Front descendant
    if (now - debounceTime >= DEBOUNCE_DELAY) {

      if (!displayEnabled) {
        // ── Réveil uniquement ─────────────────────────────────────────────
        wakeTime = now;
        if (s_onWake) s_onWake();
        Serial.print("→ Réveil de l'afficheur (");
        Serial.print(label);
        Serial.println(")");
      }
      else {
        if (now - wakeTime <= WAKE_WINDOW) {
          // ── Ignoré : appui trop proche du réveil ─────────────────────
          Serial.println("→ Ignoré (réveil récent)");
        } else {
          // ── Action normale ────────────────────────────────────────────
          if (onAction) onAction();
        }
      }

      debounceTime = now;
    }
  }

  prevState = state;
}

// ─────────────────────────────────────────────────────────────────────────────

void buttons_update(bool displayEnabled) {
  handleButton(BTN_PLUS,  s_prevPlus,  s_debounceTimePlus,  s_wakeTimePlus,
               s_onPlus,  displayEnabled, "bouton +");

  handleButton(BTN_MINUS, s_prevMinus, s_debounceTimeMinus, s_wakeTimeMinus,
               s_onMinus, displayEnabled, "bouton -");

  handleButton(BTN_RESET, s_prevReset, s_debounceTimeReset, s_wakeTimeReset,
               s_onReset, displayEnabled, "bouton RESET");
}

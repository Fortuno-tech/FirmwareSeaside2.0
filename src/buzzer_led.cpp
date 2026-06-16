#include "buzzer_led.h"
#include "config.h"

// ─── État interne ─────────────────────────────────────────────────────────────
static unsigned long s_startTime = 0;
static bool          s_active    = false;

// ─────────────────────────────────────────────────────────────────────────────

void buzzerLed_init() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN,    OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
}

// ─────────────────────────────────────────────────────────────────────────────

void buzzerLed_trigger() {
  s_active    = true;
  s_startTime = millis();
  tone(BUZZER_PIN, 1500);
  digitalWrite(LED_PIN, HIGH);
}

// ─────────────────────────────────────────────────────────────────────────────

void buzzerLed_doubleBeep() {
  tone(BUZZER_PIN, 2000, 150);
  delay(100);
  tone(BUZZER_PIN, 2000, 150);
}

// ─────────────────────────────────────────────────────────────────────────────

void buzzerLed_update() {
  if (s_active && (millis() - s_startTime >= BUZZER_LED_DURATION)) {
    noTone(BUZZER_PIN);
    digitalWrite(LED_PIN, LOW);
    s_active = false;
  }
}

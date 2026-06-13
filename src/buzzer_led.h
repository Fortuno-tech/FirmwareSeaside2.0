#ifndef BUZZER_LED_H
#define BUZZER_LED_H

#include <Arduino.h>

// ─── Initialisation ───────────────────────────────────────────────────────────
void buzzerLed_init();

// ─── Déclencher un bip court + LED (non bloquant) ────────────────────────────
void buzzerLed_trigger();

// ─── Bip double pour le reset ────────────────────────────────────────────────
// Utilise delay() — appeler uniquement sur événement reset (rare)
void buzzerLed_doubleBeep();

// ─── À appeler dans loop() pour gérer la fin du bip ─────────────────────────
void buzzerLed_update();

#endif // BUZZER_LED_H

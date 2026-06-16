#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <Arduino.h>

// ─── Initialisation ───────────────────────────────────────────────────────────
void ultrasonic_init();

// ─── Lecture distance ─────────────────────────────────────────────────────────
// Retourne la distance en cm, ou 0 si aucun écho reçu
float ultrasonic_readDistance();

#endif // ULTRASONIC_H

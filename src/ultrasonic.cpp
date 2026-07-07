#include "ultrasonic.h"
#include "config.h"

void ultrasonic_init() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

// ─────────────────────────────────────────────────────────────────────────────

float ultrasonic_readDistance() {
  // Impulsion TRIG
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Lecture écho (timeout 30 ms → ~5 m max)
  long duree = pulseIn(ECHO_PIN, HIGH, 30000);

  // FIX : pulseIn retourne 0 si timeout (aucun écho).
  
  if (duree == 0) return -1.0f;

  // Conversion en cm (vitesse son ≈ 0.034 cm/µs, aller-retour /2)
  float distance = duree * 0.034f / 2.0f;
  return distance;
}


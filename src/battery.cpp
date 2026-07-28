#include "battery.h"

void battery_init() {
  pinMode(BATTERY_POT_PIN, INPUT);
  analogSetPinAttenuation(BATTERY_POT_PIN, ADC_11db);
}

int battery_getPercent() {
  // Moyenne de plusieurs lectures pour stabiliser l'affichage.
  uint32_t total = 0;
  for (int i = 0; i < 8; i++) {
    total += analogRead(BATTERY_POT_PIN);
  }

  int raw = total / 8;
  int percent = map(raw, 0, 4095, 0, 100);
  return constrain(percent, 0, 100);
}

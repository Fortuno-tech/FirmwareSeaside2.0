#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>

// Potentiomètre de simulation batterie : curseur vers GPIO33,
// extrémités vers 3.3 V et GND.
#define BATTERY_POT_PIN 34

void battery_init();
int battery_getPercent();

#endif

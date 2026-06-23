#include "display.h"
#include "config.h"

// ─── Table des codes 7 segments (Common Anode) ───────────────────────────────
static const byte SEG_CODES[10] = {
  0b11000000, // 0
  0b11111001, // 1
  0b10100100, // 2
  0b10110000, // 3
  0b10011001, // 4
  0b10010010, // 5
  0b10000010, // 6
  0b11111000, // 7
  0b10000000, // 8
  0b10010000  // 9
};

// ─── Segment "vide" (Common Anode : tous segments éteints) ───────────────────
static const byte SEG_BLANK = 0b11111111;

// ─────────────────────────────────────────────────────────────────────────────

void display_init() {
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);

  // Éteindre tous les digits au démarrage
  display_off();
}

// ─────────────────────────────────────────────────────────────────────────────

void display_off() {
  digitalWrite(D1, LOW);
  digitalWrite(D2, LOW);
  digitalWrite(D3, LOW);
  digitalWrite(D4, LOW);
}

// ─────────────────────────────────────────────────────────────────────────────

void display_showNumber(int nombre, bool enabled) {
  if (!enabled) {
    display_off();
    return;
  }

  int milliers  = (nombre / 1000) % 10;
  int centaines = (nombre / 100)  % 10;
  int dizaines  = (nombre / 10)   % 10;
  int unites    =  nombre         % 10;

  digitalWrite(LATCH_PIN, LOW);

  // ── Digit Unités (D4) — toujours affiché ──────────────────────────────────
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, SEG_CODES[unites]);
  digitalWrite(D4, HIGH);

  // ── Digit Centaines (D2) ─────────────────────────────────────────────────
  if (nombre >= 100) {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, SEG_CODES[centaines]);
    digitalWrite(D2, HIGH);
  } else {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, SEG_BLANK);
    digitalWrite(D2, LOW);
  }

  // ── Digit Dizaines (D3) ───────────────────────────────────────────────────
  if (nombre >= 10) {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, SEG_CODES[dizaines]);
    digitalWrite(D3, HIGH);
  } else {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, SEG_BLANK);
    digitalWrite(D3, LOW);
  }

  // ── Digit Milliers (D1) ──────────────────────────────────────────────────
  if (nombre >= 1000) {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, SEG_CODES[milliers]);
    digitalWrite(D1, HIGH);
  } else {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, SEG_BLANK);
    digitalWrite(D1, LOW);
  }

  digitalWrite(LATCH_PIN, HIGH);
}

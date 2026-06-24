#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

// ─── Initialisation ───────────────────────────────────────────────────────────
// Appeler une fois dans setup() pour configurer les broches
void display_init();

// ─── Affichage ────────────────────────────────────────────────────────────────
// Affiche un entier (0–9999) sur les 4 digits 7 segments
// Si enabled == false, éteint tous les afficheurs
void display_showNumber(int nombre, bool enabled);

// ─── Extinction complète ──────────────────────────────────────────────────────
void display_off();

// ─── Mise à jour immédiate ───────────────────────────────────────────────────
void triggerImmediateDisplayUpdate();

#endif // DISPLAY_H

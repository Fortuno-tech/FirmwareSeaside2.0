#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>

// ─── Callback appelé par le module boutons lors d'une action validée ──────────
// L'application enregistre ses handlers via buttons_setCallbacks()
typedef void (*ButtonCallback)();

// ─── Enregistrement des callbacks ────────────────────────────────────────────
// onPlus  : appelé quand BTN_PLUS est pressé (afficheur déjà allumé)
// onMinus : appelé quand BTN_MINUS est pressé
// onReset : appelé quand BTN_RESET est pressé
// onWake  : appelé quand n'importe quel bouton réveille l'afficheur
void buttons_setCallbacks(ButtonCallback onPlus,
                          ButtonCallback onMinus,
                          ButtonCallback onReset,
                          ButtonCallback onWake);

// ─── Initialisation ───────────────────────────────────────────────────────────
void buttons_init();

// ─── À appeler dans loop() ───────────────────────────────────────────────────
// Passe l'état courant de l'afficheur pour distinguer réveil vs action
void buttons_update(bool displayEnabled);

#endif // BUTTONS_H

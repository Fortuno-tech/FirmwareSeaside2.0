#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>

// ─── Initialisation ───────────────────────────────────────────────────────────
// Monte LittleFS. Retourne true si succès.
bool storage_init();

// ─── Lecture ──────────────────────────────────────────────────────────────────
// Charge la valeur sauvegardée. Retourne 0 si aucun fichier trouvé.
int storage_load();

// ─── Écriture immédiate ───────────────────────────────────────────────────────
void storage_save(int valeur);

// ─── Sauvegarde différée ──────────────────────────────────────────────────────
// Appeler storage_markDirty() après chaque modification du compteur.
// Appeler storage_update() dans loop() : déclenche la sauvegarde après SAVE_DELAY.
void storage_markDirty();
void storage_update(int valeur);

// ─── Formatage (debug) ───────────────────────────────────────────────────────
void storage_format();

#endif // STORAGE_H

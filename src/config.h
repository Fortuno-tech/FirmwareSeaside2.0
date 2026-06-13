#ifndef CONFIG_H
#define CONFIG_H

// ─── Shift Register (74HC595) ─────────────────────────────────────────────────
#define DATA_PIN   23
#define LATCH_PIN  22
#define CLOCK_PIN  21

// ─── Capteur ultrason ─────────────────────────────────────────────────────────
#define TRIG_PIN   14
#define ECHO_PIN   32
#define SEUIL      80       // Distance seuil en cm

// ─── Buzzer et LED ────────────────────────────────────────────────────────────
#define BUZZER_PIN 5
#define LED_PIN    12

// ─── Afficheurs 7 segments (Common Anode) ────────────────────────────────────
#define D1  4   // Milliers  (gauche)
#define D2  15  // Centaines
#define D3  2   // Dizaines
#define D4  13  // Unités    (droite)

// ─── Boutons ─────────────────────────────────────────────────────────────────
#define BTN_PLUS   25
#define BTN_MINUS  33
#define BTN_RESET  27

// ─── Timings ─────────────────────────────────────────────────────────────────
#define INACTIVITY_TIMEOUT   20000UL  // Veille après 20 s d'inactivité (ms)
#define LOOP_INTERVAL          400UL  // Rafraîchissement afficheur (ms)
#define DEBOUNCE_DELAY         200UL  // Anti-rebond boutons (ms)
#define WAKE_WINDOW            500UL  // Fenêtre réveil vs action (ms)
#define SAVE_DELAY            2000UL  // Délai sauvegarde différée (ms)
#define BUZZER_LED_DURATION    300UL  // Durée buzzer/LED (ms)

// ─── Fichier de sauvegarde ───────────────────────────────────────────────────
#define SAVE_FILE "/compteur.txt"

#endif // CONFIG_H

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// â”€â”€â”€ Shift Register (74HC595) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
#define DATA_PIN   23
#define LATCH_PIN  22
#define CLOCK_PIN  21

// â”€â”€â”€ Capteur ultrason â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
#define TRIG_PIN   14
#define ECHO_PIN   32
#define SEUIL      80       // Distance seuil en cm

// â”€â”€â”€ Buzzer et LED â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
#define BUZZER_PIN 5
#define LED_PIN    12

// â”€â”€â”€ Afficheurs 7 segments (Common Anode) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
#define D1  4   // Milliers  (gauche)
#define D2  15  // Centaines
#define D3  2   // Dizaines
#define D4  13  // Unités    (droite)

// â”€â”€â”€ Boutons â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€        
#define BTN_PLUS   25
#define BTN_MINUS  33
#define BTN_RESET  27

// â”€â”€â”€ Timings â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€        
#define INACTIVITY_TIMEOUT   20000UL  // Veille après 20 s d'inactivité (ms)
#define LOOP_INTERVAL          400UL  // Rafraîchissement afficheur (ms)
#define DEBOUNCE_DELAY         200UL  // Anti-rebond boutons (ms)
#define WAKE_WINDOW            500UL  // Fenêtre réveil vs action (ms)
#define SAVE_DELAY            2000UL  // Délai sauvegarde différée (ms)
#define BUZZER_LED_DURATION    300UL  // Durée buzzer/LED (ms)

// â”€â”€â”€ Fichier de sauvegarde â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
#define SAVE_FILE "/compteur.txt"

// â”€â”€â”€ Variables globales (externes) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
extern String apSSID;
extern String apPassword;
extern int totalPersonnes;
extern int personnesActuelles;
extern String moduleRole;
extern String masterMAC;
extern String staSSID;
extern String staPassword;
extern String mqttServer;
extern int mqttPort;
extern int compteur;
extern bool mqttTriggerSetup;

#endif // CONFIG_H


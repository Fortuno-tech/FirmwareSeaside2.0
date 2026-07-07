#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

//              Shift Register (74HC595)                                                                                                                                                                                                 â”€
#define DATA_PIN   23
#define LATCH_PIN  22
#define CLOCK_PIN  21

//              Capteur ultrason                                                                                                                                                                                                                                     
#define TRIG_PIN   14
#define ECHO_PIN   32
#define SEUIL_DEFAULT  80    // Valeur par défaut du seuil (cm)
#define SEUIL_MIN       10   // Seuil minimum autorisé (cm)
#define SEUIL_MAX      100   // Seuil maximum autorisé (cm)

//              Buzzer et LED                                                                                                                                                                                                                                                 
#define BUZZER_PIN 5
#define LED_PIN    12

//              Afficheurs 7 segments (Common Anode)                                                                                                                                                 
#define D1  4   // Milliers  (gauche)
#define D2  15  // Centaines
#define D3  2   // Dizaines
#define D4  13  // Unités    (droite)

//              Boutons                                                                                                                                                                                                                                                             â”€â”€        
#define BTN_PLUS   27
#define BTN_MINUS  26
#define BTN_RESET  25

//              Timings                                                                                                                                                                                                                                                             â”€â”€        
#define INACTIVITY_TIMEOUT   20000UL  // Veille après 20 s d'inactivité (ms)
#define LOOP_INTERVAL          400UL  // Rafraîchissement afficheur (ms)
#define DEBOUNCE_DELAY         200UL  // Anti-rebond boutons (ms)
#define WAKE_WINDOW            500UL  // Fenêtre réveil vs action (ms)
#define SAVE_DELAY            2000UL  // Délai sauvegarde différée (ms)
#define BUZZER_LED_DURATION    300UL  // Durée buzzer/LED (ms)

//              Fichier de sauvegarde                                                                                                                                                                                                             
#define SAVE_FILE "/compteur.txt"

//              Variables globales (externes)                                                                               
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
extern String mqttUser;
extern String mqttPassword;
extern int compteur;
extern int seuil;
extern bool mqttTriggerSetup;
extern bool requestReboot;
extern unsigned long rebootTimer;
extern String moduleId;          // Identifiant unique du module (ex: "Master", "Slave-1")
extern bool isMasterConfigured;  // true si un rôle a déjà été attribué (sauvegardé en flash)
extern String licenseCode;       // Code de licence du produit (Master)
extern String licenseDate;       // Date de création de la licence
extern int licenseDuration;      // Durée de validité en jours
extern String licenseExpiry;     // Date d'expiration de la licence
extern bool shouldFormat;

#endif // CONFIG_H


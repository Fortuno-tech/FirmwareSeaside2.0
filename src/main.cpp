#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "config.h"
#include "wifi_ap.h"
#include "webserver.h"
#include "espnow.h"
#include "ota.h"
#include "mqtt.h"

// --- Configuration Hardware (depuis main_anito.cpp) ---
#define DATA_PIN 23
#define LATCH_PIN 22
#define CLOCK_PIN 21
#define TRIG_PIN 14
#define ECHO_PIN 32
#define BUZZER_PIN 5
#define LED 12 

// Pins des afficheurs 7 segments (Common Anode)
#define D1 4   // Milliers (gauche)
#define D2 15  // Dizaines
#define D3 2   // Centaines
#define D4 13  // Unites (droite)

// Boutons
#define BTN_PLUS 25    // Bouton incrementer
#define BTN_MINUS 33   // Bouton decrementer
#define BTN_RESET 27   // Bouton remise a zero

#define SEUIL 80
#define INACTIVITY_TIMEOUT 20000UL

// WiFi Internet pour MQTT (Valeurs par defaut, configurables via Web)
const char* STA_SSID_DEFAULT = "TonWiFi";
const char* STA_PASS_DEFAULT = "TonMotDePasse";

// --- Variables Globales (depuis main_anito.cpp) ---
byte seg[10] = {
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

bool objetDetecte = false;
bool ancienEtatPlus = HIGH;
bool ancienEtatMinus = HIGH;
bool ancienEtatReset = HIGH;

unsigned long lastActivityTime = 0;
bool displayEnabled = true;

unsigned long lastLoopTime = 0;
const unsigned long LOOP_INTERVAL = 400;

unsigned long buzzerLedStartTime = 0;
bool buzzerLedActive = false;
const unsigned long BUZZER_LED_DURATION = 300;

unsigned long lastDebounceTimePlus = 0;
unsigned long lastDebounceTimeMinus = 0;
unsigned long lastDebounceTimeReset = 0;
const unsigned long DEBOUNCE_DELAY = 200;

unsigned long lastWakeUpTimePlus = 0;
unsigned long lastWakeUpTimeMinus = 0;
unsigned long lastWakeUpTimeReset = 0;
const unsigned long WAKE_WINDOW = 500; 

// --- Fonctions Helper (depuis main_anito.cpp) ---

float lireDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duree = pulseIn(ECHO_PIN, HIGH, 30000);
  float distance = duree * 0.034 / 2;
  return distance;
}

void activerBuzzerLed() {
  buzzerLedActive = true;
  buzzerLedStartTime = millis();
  tone(BUZZER_PIN, 1500);
  digitalWrite(LED, HIGH);
}

void gererBuzzerLed() {
  if (buzzerLedActive) {
    if (millis() - buzzerLedStartTime >= BUZZER_LED_DURATION) {
      noTone(BUZZER_PIN);
      digitalWrite(LED, LOW);
      buzzerLedActive = false;
    }
  }
}

void afficherNombre(int nombre) {
  if (!displayEnabled) {
    digitalWrite(D1, LOW);
    digitalWrite(D2, LOW);
    digitalWrite(D3, LOW);
    digitalWrite(D4, LOW);
    return;
  }

  int milliers = (nombre / 1000) % 10;
  int centaines = (nombre / 100) % 10;
  int dizaines = (nombre / 10) % 10;
  int unites = nombre % 10;

  digitalWrite(LATCH_PIN, LOW);

  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, seg[unites]);
  digitalWrite(D4, HIGH);

  if (nombre >= 10) {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, seg[dizaines]);
    digitalWrite(D2, HIGH);
  } else {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, 0b11111111);
    digitalWrite(D2, LOW);
  }

  if (nombre >= 100) {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, seg[centaines]);
    digitalWrite(D3, HIGH);
  } else {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, 0b11111111);
    digitalWrite(D3, LOW);
  }

  if (nombre >= 1000) {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, seg[milliers]);
    digitalWrite(D1, HIGH);
  } else {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, 0b11111111);
    digitalWrite(D1, LOW);
  }

  digitalWrite(LATCH_PIN, HIGH);
}

void gererDetectionUltrason() {
  float distance = lireDistance();

  if (distance > 0 && distance < SEUIL) {
    if (!objetDetecte) {
      personnesActuelles++;
      totalPersonnes++;
      objetDetecte = true;
      lastActivityTime = millis();
      displayEnabled = true;
      activerBuzzerLed();

      Serial.print("-> Obstacle detecte ! Compteur = ");
      Serial.println(personnesActuelles);
    }
  } else {
    objetDetecte = false;
  }
}

void gererBoutons() {
  unsigned long currentTime = millis();

  // Bouton +
  bool etatPlus = digitalRead(BTN_PLUS);
  if (ancienEtatPlus == HIGH && etatPlus == LOW) {
    if (currentTime - lastDebounceTimePlus >= DEBOUNCE_DELAY) {
      if (!displayEnabled) {
        displayEnabled = true;
        lastActivityTime = currentTime;
        lastWakeUpTimePlus = currentTime;
      } else {
        if (currentTime - lastWakeUpTimePlus > WAKE_WINDOW) {
          personnesActuelles++;
          totalPersonnes++;
          lastActivityTime = currentTime;
          Serial.print("-> Bouton + | Compteur = ");
          Serial.println(personnesActuelles);
        }
      }
      lastDebounceTimePlus = currentTime;
    }
  }
  ancienEtatPlus = etatPlus;

  // Bouton -
  bool etatMinus = digitalRead(BTN_MINUS);
  if (ancienEtatMinus == HIGH && etatMinus == LOW) {
    if (currentTime - lastDebounceTimeMinus >= DEBOUNCE_DELAY) {
      if (!displayEnabled) {
        displayEnabled = true;
        lastActivityTime = currentTime;
        lastWakeUpTimeMinus = currentTime;
      } else {
        if (currentTime - lastWakeUpTimeMinus > WAKE_WINDOW) {
          if (personnesActuelles > 0) personnesActuelles--;
          lastActivityTime = currentTime;
          Serial.print("-> Bouton - | Compteur = ");
          Serial.println(personnesActuelles);
        }
      }
      lastDebounceTimeMinus = currentTime;
    }
  }
  ancienEtatMinus = etatMinus;

  // Bouton RESET
  bool etatReset = digitalRead(BTN_RESET);
  if (ancienEtatReset == HIGH && etatReset == LOW) {
    if (currentTime - lastDebounceTimeReset >= DEBOUNCE_DELAY) {
      if (!displayEnabled) {
        displayEnabled = true;
        lastActivityTime = currentTime;
        lastWakeUpTimeReset = currentTime;
      } else {
        if (currentTime - lastWakeUpTimeReset > WAKE_WINDOW) {
          personnesActuelles = 0;
          lastActivityTime = currentTime;
          Serial.println("!!! BOUTON RESET - COMPTEUR REMIS A ZERO !!!");
          tone(BUZZER_PIN, 2000, 150);
          delay(100);
          tone(BUZZER_PIN, 2000, 150);
        }
      }
      lastDebounceTimeReset = currentTime;
    }
  }
  ancienEtatReset = etatReset;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== Seaside 2.0 - Boot (Integrated with Anito) ===");

  // Hardware Pins Setup
  pinMode(DATA_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(BTN_PLUS, INPUT_PULLUP);
  pinMode(BTN_MINUS, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);

  digitalWrite(D1, LOW);
  digitalWrite(D2, LOW);
  digitalWrite(D3, LOW);
  digitalWrite(D4, LOW);

  lastActivityTime = millis();
  lastLoopTime = millis();

  if (!LittleFS.begin()) {
    Serial.println("Erreur LittleFS !");
  } else {
    Serial.println("LittleFS OK");
  }

  setupAP();
  setupServer();
  setupOTA();

  if (moduleRole == "master") {
    setupESPNOW_Master();
    // Utilise staSSID/staPassword de la config si disponibles
    const char* ssid = (staSSID.length() > 0) ? staSSID.c_str() : STA_SSID_DEFAULT;
    const char* pass = (staPassword.length() > 0) ? staPassword.c_str() : STA_PASS_DEFAULT;
    setupMQTT(ssid, pass);
  } else {
    setupESPNOW_Slave();
  }

  Serial.print("Role : ");
  Serial.println(moduleRole);
}

void loop() {
  handleOTA();
  handleMQTT();

  unsigned long currentTime = millis();

  // Gestion Hardware Anito
  gererBuzzerLed();
  gererBoutons();
  gererDetectionUltrason();

  // Gestion de la mise en veille
  if (currentTime - lastActivityTime >= INACTIVITY_TIMEOUT) {
    displayEnabled = false;
  } else {
    displayEnabled = true;
  }

  // Rafraichissement de l'affichage (non-bloquant)
  if (currentTime - lastLoopTime >= LOOP_INTERVAL) {
    afficherNombre(personnesActuelles);
    lastLoopTime = currentTime;
  }

  // Communication Reseau
  if (moduleRole == "slave") {
    // Le Slave envoie les donnees au Master toutes les 2 secondes
    static unsigned long lastSend = 0;
    if (currentTime - lastSend > 2000) {
      lastSend = currentTime;
      
      if (masterMAC != "00:00:00:00:00:00") {
        uint8_t macBytes[6];
        int values[6];
        if (sscanf(masterMAC.c_str(), "%%x:%%x:%%x:%%x:%%x:%%x",
            &values[0], &values[1], &values[2],
            &values[3], &values[4], &values[5]) == 6) {
          for (int i = 0; i < 6; i++) macBytes[i] = (uint8_t)values[i];
          espnow_sendData(macBytes, personnesActuelles);
          Serial.println("Slave: Donnees envoyees au Master");
        }
      }
    }
  } else {
    // Le Master publie sur MQTT toutes les 5 secondes
    static unsigned long lastPublish = 0;
    if (currentTime - lastPublish > 5000) {
      lastPublish = currentTime;
      mqttPublishCount(totalPersonnes, personnesActuelles);
    }
  }
}

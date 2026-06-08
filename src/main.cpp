#include <Arduino.h>

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
#define D4 13  // Unités (droite)

// Boutons
#define BTN_PLUS 25    // Bouton incrémenter
#define BTN_MINUS 33   // Bouton décrémenter
#define BTN_RESET 27   // Bouton remise à zéro

#define SEUIL 80 
#define INACTIVITY_TIMEOUT 20000UL 

// Codes 7 segments pour Common Anode
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

int compteur = 0;
bool objetDetecte = false;

bool ancienEtatPlus = HIGH;
bool ancienEtatMinus = HIGH;
bool ancienEtatReset = HIGH;

unsigned long lastActivityTime = 0;
bool displayEnabled = true;

// Variables pour la gestion non-bloquante
unsigned long lastLoopTime = 0;
const unsigned long LOOP_INTERVAL = 400;

// Variables pour la gestion du buzzer/LED
unsigned long buzzerLedStartTime = 0;
bool buzzerLedActive = false;
const unsigned long BUZZER_LED_DURATION = 300;

// Variables pour l'anti-rebond des boutons
unsigned long lastDebounceTimePlus = 0;
unsigned long lastDebounceTimeMinus = 0;
unsigned long lastDebounceTimeReset = 0;
const unsigned long DEBOUNCE_DELAY = 200;

// NOUVEAU : Variables pour la gestion du réveil des boutons
unsigned long lastWakeUpTimePlus = 0;
unsigned long lastWakeUpTimeMinus = 0;
unsigned long lastWakeUpTimeReset = 0;
const unsigned long WAKE_WINDOW = 500; // Fenêtre de 500ms pour réveil vs action

void setup() {
  Serial.begin(115200);
  
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

  Serial.println("=== Systeme de comptage avec veille ===");
  Serial.println("Veille automatique après 20 secondes d'inactivité");
  Serial.println("Boutons: + (pin25), - (pin33), RESET (pin27)");
}

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
      compteur++;
      objetDetecte = true;
      lastActivityTime = millis();
      displayEnabled = true;
      activerBuzzerLed();

      Serial.print("→ Obstacle détecté ! Compteur = ");
      Serial.println(compteur);
    }
  } else {
    objetDetecte = false;
  }
}

void gererBoutons() {
  unsigned long currentTime = millis();
  
  // Bouton + (pin 25)
  bool etatPlus = digitalRead(BTN_PLUS);
  if (ancienEtatPlus == HIGH && etatPlus == LOW) {
    if (currentTime - lastDebounceTimePlus >= DEBOUNCE_DELAY) {
      
      // Cas 1: L'afficheur est éteint -> on le réveille seulement
      if (!displayEnabled) {
        displayEnabled = true;
        lastActivityTime = currentTime;
        lastWakeUpTimePlus = currentTime;  // Enregistre le moment du réveil
        Serial.println("→ Réveil de l'afficheur (bouton +)");
      }
      // Cas 2: L'afficheur est allumé
      else {
        // Vérifie si on vient juste de réveiller l'afficheur avec ce bouton
        if (currentTime - lastWakeUpTimePlus <= WAKE_WINDOW) {
          // Dans la fenêtre de réveil, on ne fait qu'ignorer pour éviter double action
          Serial.println("→ Ignoré (réveil récent)");
        } else {
          // Action normale : incrémentation
          compteur++;
          lastActivityTime = currentTime;
          Serial.print("→ Bouton + | Compteur = ");
          Serial.println(compteur);
        }
      }
      
      lastDebounceTimePlus = currentTime;
    }
  }
  ancienEtatPlus = etatPlus;

  // Bouton - (pin 33)
  bool etatMinus = digitalRead(BTN_MINUS);
  if (ancienEtatMinus == HIGH && etatMinus == LOW) {
    if (currentTime - lastDebounceTimeMinus >= DEBOUNCE_DELAY) {
      
      // Cas 1: L'afficheur est éteint -> on le réveille seulement
      if (!displayEnabled) {
        displayEnabled = true;
        lastActivityTime = currentTime;
        lastWakeUpTimeMinus = currentTime;  // Enregistre le moment du réveil
        Serial.println("→ Réveil de l'afficheur (bouton -)");
      }
      // Cas 2: L'afficheur est allumé
      else {
        // Vérifie si on vient juste de réveiller l'afficheur avec ce bouton
        if (currentTime - lastWakeUpTimeMinus <= WAKE_WINDOW) {
          Serial.println("→ Ignoré (réveil récent)");
        } else {
          // Action normale : décrémentation
          if (compteur > 0) compteur--;
          lastActivityTime = currentTime;
          Serial.print("→ Bouton - | Compteur = ");
          Serial.println(compteur);
        }
      }
      
      lastDebounceTimeMinus = currentTime;
    }
  }
  ancienEtatMinus = etatMinus;

  // Bouton RESET (pin 27)
  bool etatReset = digitalRead(BTN_RESET);
  if (ancienEtatReset == HIGH && etatReset == LOW) {
    if (currentTime - lastDebounceTimeReset >= DEBOUNCE_DELAY) {
      
      // Cas 1: L'afficheur est éteint -> on le réveille seulement
      if (!displayEnabled) {
        displayEnabled = true;
        lastActivityTime = currentTime;
        lastWakeUpTimeReset = currentTime;  // Enregistre le moment du réveil
        Serial.println("→ Réveil de l'afficheur (bouton RESET)");
      }
      // Cas 2: L'afficheur est allumé
      else {
        // Vérifie si on vient juste de réveiller l'afficheur avec ce bouton
        if (currentTime - lastWakeUpTimeReset <= WAKE_WINDOW) {
          Serial.println("→ Ignoré (réveil récent)");
        } else {
          // Action normale : reset
          compteur = 0;
          lastActivityTime = currentTime;
          Serial.println("!!! BOUTON RESET - COMPTEUR REMIS À ZÉRO !!!");
          // Signal sonore spécifique pour le reset (2 bips)
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

void loop() {
  unsigned long currentTime = millis();
  
  gererBuzzerLed();
  gererBoutons();
  gererDetectionUltrason();

  // Gestion de la mise en veille
  if (currentTime - lastActivityTime >= INACTIVITY_TIMEOUT) {
    displayEnabled = false;
  } else {
    displayEnabled = true;
  }

  if (currentTime - lastLoopTime >= LOOP_INTERVAL) {
    afficherNombre(compteur);
    lastLoopTime = currentTime;
  }
}
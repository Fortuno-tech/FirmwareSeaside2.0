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
#define BTN_PLUS 27
#define BTN_MINUS 26 

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

unsigned long lastActivityTime = 0;
bool displayEnabled = true;

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

  digitalWrite(D1, LOW);
  digitalWrite(D2, LOW);
  digitalWrite(D3, LOW);
  digitalWrite(D4, LOW);

  lastActivityTime = millis();

  Serial.println("=== Systeme de comptage avec veille ===");
  Serial.println("Veille automatique après 20 secondes d'inactivité");
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

void afficherNombre(int nombre) {
  if (!displayEnabled) {
    digitalWrite(D1, LOW);
    digitalWrite(D2, LOW);
    digitalWrite(D3, LOW);
    digitalWrite(D4, LOW);
    return;
  }

  // Extraire chaque chiffre selon l'ordre physique d'affichage
  // Ordre physique de gauche à droite : D1 (Milliers) → D3 (Centaines) → D2 (Dizaines) → D4 (Unités)
  int milliers = (nombre / 1000) % 10;   // D1
  int centaines = (nombre / 100) % 10;   // D3
  int dizaines = (nombre / 10) % 10;     // D2
  int unites = nombre % 10;              // D4

  digitalWrite(LATCH_PIN, LOW);

  // Ordre d'envoi (inverse du chaînage physique)
  // Chaînage : sr1 → sr3 → sr2 → sr4
  // Donc envoi : sr4, sr2, sr3, sr1
  
  // 1. Dernier registre → sr4 (Unités - D4)
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, seg[unites]);
  digitalWrite(D4, HIGH);

  // 2. Avant-dernier → sr2 (Dizaines - D2)
  if (nombre >= 10) {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, seg[dizaines]);
    digitalWrite(D2, HIGH);
  } else {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, 0b11111111);
    digitalWrite(D2, LOW);
  }

  // 3. Deuxième → sr3 (Centaines - D3)
  if (nombre >= 100) {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, seg[centaines]);
    digitalWrite(D3, HIGH);
  } else {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, 0b11111111);
    digitalWrite(D3, LOW);
  }

  // 4. Premier registre → sr1 (Milliers - D1)
  if (nombre >= 1000) {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, seg[milliers]);
    digitalWrite(D1, HIGH);
  } else {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, 0b11111111);
    digitalWrite(D1, LOW);
  }

  digitalWrite(LATCH_PIN, HIGH);
}

void loop() {
  unsigned long currentTime = millis();
  float distance = lireDistance();

  // Détection ultrason
  if (distance > 0 && distance < SEUIL) {
    if (!objetDetecte) {
      compteur++;
      objetDetecte = true;
      lastActivityTime = currentTime;
      displayEnabled = true;

      tone(BUZZER_PIN, 1500, 300);
      digitalWrite(LED, HIGH);
      delay(300);
      digitalWrite(LED, LOW);

      Serial.print("→ Obstacle détecté ! Compteur = ");
      Serial.println(compteur);
    }
  } else {
    objetDetecte = false;
  }

  // Bouton +
  bool etatPlus = digitalRead(BTN_PLUS);
  if (ancienEtatPlus == HIGH && etatPlus == LOW) {
    compteur++;
    lastActivityTime = currentTime;
    displayEnabled = true;
    Serial.print("→ Bouton + | Compteur = ");
    Serial.println(compteur);
    delay(200);
  }
  ancienEtatPlus = etatPlus;

  // Bouton -
  bool etatMinus = digitalRead(BTN_MINUS);
  if (ancienEtatMinus == HIGH && etatMinus == LOW) {
    if (compteur > 0) compteur--;
    lastActivityTime = currentTime;
    displayEnabled = true;
    Serial.print("→ Bouton - | Compteur = ");
    Serial.println(compteur);
    delay(200);
  }
  ancienEtatMinus = etatMinus;

  // Gestion de la veille
  if (currentTime - lastActivityTime >= INACTIVITY_TIMEOUT) {
    displayEnabled = false;
  } else {
    displayEnabled = true;
  }

  afficherNombre(compteur);
  delay(400);
}
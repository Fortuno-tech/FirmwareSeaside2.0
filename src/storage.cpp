#include "storage.h"
#include "config.h"
#include <LittleFS.h>

// ─── État interne ─────────────────────────────────────────────────────────────
static bool          s_needSave    = false;
static unsigned long s_lastChanged = 0;

// ─────────────────────────────────────────────────────────────────────────────

bool storage_init() {
  if (!LittleFS.begin()) {
    Serial.println("✗ Erreur: montage LittleFS échoué");
    return false;
  }
  Serial.println("✓ LittleFS monté avec succès");
  return true;
}

// ─────────────────────────────────────────────────────────────────────────────

int storage_load() {
  if (!LittleFS.exists(SAVE_FILE)) {
    Serial.println("ℹ Aucune sauvegarde trouvée, compteur initialisé à 0");
    return 0;
  }

  File file = LittleFS.open(SAVE_FILE, "r");
  if (!file) {
    Serial.println("✗ Erreur: impossible d'ouvrir le fichier pour lecture");
    return 0;
  }

  int valeur = file.readString().toInt();
  file.close();
  Serial.println("✓ Compteur chargé: " + String(valeur));
  return valeur;
}

// ─────────────────────────────────────────────────────────────────────────────

void storage_save(int valeur) {
  File file = LittleFS.open(SAVE_FILE, "w");
  if (!file) {
    Serial.println("✗ Erreur: impossible d'ouvrir le fichier pour écriture");
    return;
  }
  file.println(valeur);
  file.close();
  s_needSave = false;
  Serial.println("✓ Compteur sauvegardé: " + String(valeur));
}

// ─────────────────────────────────────────────────────────────────────────────

void storage_markDirty() {
  s_needSave    = true;
  s_lastChanged = millis();
}

// ─────────────────────────────────────────────────────────────────────────────

void storage_update(int valeur) {
  if (s_needSave && (millis() - s_lastChanged >= SAVE_DELAY)) {
    storage_save(valeur);
  }
}

// ─────────────────────────────────────────────────────────────────────────────

void storage_format() {
  Serial.println("Formatage de LittleFS...");
  if (LittleFS.format()) {
    Serial.println("✓ LittleFS formaté avec succès");
  } else {
    Serial.println("✗ Erreur lors du formatage");
  }
}

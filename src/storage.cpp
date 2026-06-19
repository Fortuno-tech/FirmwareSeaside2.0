#include "storage.h"
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>


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

#define CONFIG_FILE "/config.json"

bool storage_loadConfig() {
  if (!LittleFS.exists(CONFIG_FILE)) {
    Serial.println("ℹ Aucun fichier de configuration trouvé. Utilisation des valeurs par défaut.");
    return false;
  }

  File file = LittleFS.open(CONFIG_FILE, "r");
  if (!file) {
    Serial.println("✗ Erreur: impossible d'ouvrir config.json pour lecture");
    return false;
  }

  StaticJsonDocument<600> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.println("✗ Erreur: échec de la désérialisation de config.json");
    return false;
  }

  if (doc.containsKey("apSSID")) apSSID = doc["apSSID"].as<String>();
  if (doc.containsKey("apPassword")) apPassword = doc["apPassword"].as<String>();
  if (doc.containsKey("moduleRole")) moduleRole = doc["moduleRole"].as<String>();
  if (doc.containsKey("masterMAC")) masterMAC = doc["masterMAC"].as<String>();
  if (doc.containsKey("staSSID")) staSSID = doc["staSSID"].as<String>();
  if (doc.containsKey("staPassword")) staPassword = doc["staPassword"].as<String>();
  if (doc.containsKey("mqttServer")) mqttServer = doc["mqttServer"].as<String>();
  if (doc.containsKey("mqttPort")) mqttPort = doc["mqttPort"].as<int>();
  if (doc.containsKey("seuil")) {
    int val = doc["seuil"].as<int>();
    if (val >= SEUIL_MIN && val <= SEUIL_MAX) seuil = val;
  }

  Serial.println("✓ Configuration chargée avec succès");
  return true;
}

void storage_saveConfig() {
  File file = LittleFS.open(CONFIG_FILE, "w");
  if (!file) {
    Serial.println("✗ Erreur: impossible d'ouvrir config.json pour écriture");
    return;
  }

  StaticJsonDocument<600> doc;
  doc["apSSID"] = apSSID;
  doc["apPassword"] = apPassword;
  doc["moduleRole"] = moduleRole;
  doc["masterMAC"] = masterMAC;
  doc["staSSID"] = staSSID;
  doc["staPassword"] = staPassword;
  doc["mqttServer"] = mqttServer;
  doc["mqttPort"] = mqttPort;
  doc["seuil"] = seuil;

  if (serializeJson(doc, file) == 0) {
    Serial.println("✗ Erreur: échec de l'écriture dans config.json");
  } else {
    Serial.println("✓ Configuration sauvegardée avec succès");
  }
  file.close();
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

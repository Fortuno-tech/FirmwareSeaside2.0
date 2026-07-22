#include "storage.h"
#include <WiFi.h>
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <time.h>
#include <esp_system.h>


// ─── État interne ─────────────────────────────────────────────────────────────
static bool          s_needSave    = false;
static unsigned long s_lastChanged = 0;

// ─────────────────────────────────────────────────────────────────────────────

bool storage_init() {
  if (!LittleFS.begin(true)) {
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

  StaticJsonDocument<1024> doc;
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
  if (doc.containsKey("mqttUser")) mqttUser = doc["mqttUser"].as<String>();
  if (doc.containsKey("mqttPassword")) mqttPassword = doc["mqttPassword"].as<String>();
  if (doc.containsKey("moduleId")) moduleId = doc["moduleId"].as<String>();
  if (doc.containsKey("isMasterConfigured")) isMasterConfigured = doc["isMasterConfigured"].as<bool>();
  if (doc.containsKey("licenseCode"))        licenseCode = doc["licenseCode"].as<String>();
  if (doc.containsKey("licenseDate"))        licenseDate = doc["licenseDate"].as<String>();
  if (doc.containsKey("licenseDuration"))    licenseDuration = doc["licenseDuration"].as<int>();
  if (doc.containsKey("licenseExpiry"))      licenseExpiry = doc["licenseExpiry"].as<String>();
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

  StaticJsonDocument<1024> doc;
  doc["apSSID"] = apSSID;
  doc["apPassword"] = apPassword;
  doc["moduleRole"] = moduleRole;
  doc["masterMAC"] = masterMAC;
  doc["staSSID"] = staSSID;
  doc["staPassword"] = staPassword;
  doc["mqttServer"] = mqttServer;
  doc["mqttPort"] = mqttPort;
  doc["mqttUser"] = mqttUser;
  doc["mqttPassword"] = mqttPassword;
  doc["seuil"] = seuil;
  doc["moduleId"] = moduleId;
  doc["isMasterConfigured"] = isMasterConfigured;
  doc["licenseCode"] = licenseCode;
  doc["licenseDate"] = licenseDate;
  doc["licenseDuration"] = licenseDuration;
  doc["licenseExpiry"] = licenseExpiry;

  if (serializeJson(doc, file) == 0) {
    Serial.println("✗ Erreur: échec de l'écriture dans config.json");
  } else {
    Serial.println("✓ Configuration sauvegardée avec succès");
  }
  file.close();
}

// ---------- License helpers ----------
// Generate a license based on device MAC, current time and duration (days)
bool storage_generateLicense(int durationDays) {
  if (durationDays < 0 || durationDays > 3650) return false;

  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toUpperCase();

  time_t start = time(nullptr);
  // Sans horloge synchronisée, l'interface devra fournir la date ISO.
  // Ne pas fabriquer une date d'expiration basée sur millis(), car elle
  // deviendrait fausse après un redémarrage.
  if (start < 1700000000) return false;

  struct tm startTm;
  gmtime_r(&start, &startTm);
  char startIso[25];
  strftime(startIso, sizeof(startIso), "%Y-%m-%dT%H:%M:%SZ", &startTm);

  time_t expiry = start + (time_t)durationDays * 86400;
  char expiryIso[25] = "";
  if (durationDays > 0) {
    struct tm expiryTm;
    gmtime_r(&expiry, &expiryTm);
    strftime(expiryIso, sizeof(expiryIso), "%Y-%m-%dT%H:%M:%SZ", &expiryTm);
  }

  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%04lX", (unsigned long)(esp_random() & 0xFFFF));
  String expiryMark = durationDays == 0 ? "00000000" : String(expiryIso).substring(0, 10);
  licenseCode = "SMC-" + mac + "-" + expiryMark + "-" + suffix;
  licenseDate = String(startIso);
  licenseDuration = durationDays;
  licenseExpiry = durationDays == 0 ? "" : String(expiryIso);
  // Persist to config
  storage_saveConfig();
  Serial.printf("[License] Generated code=%s, start=%s, duration=%d, expiry=%s\n",
                licenseCode.c_str(), licenseDate.c_str(), licenseDuration, licenseExpiry.c_str());
  return true;
}

// Check if stored license is still valid (based on expiry timestamp)
bool storage_isLicenseValid() {
  if (licenseCode.length() == 0) return false;
  if (licenseDuration == 0) return true;
  if (licenseExpiry.length() == 0) return false;

  // Legacy licenses used a millisecond deadline relative to the current boot.
  bool numericExpiry = true;
  for (size_t i = 0; i < licenseExpiry.length(); ++i) {
    if (!isDigit(licenseExpiry[i])) {
      numericExpiry = false;
      break;
    }
  }
  if (numericExpiry) {
    unsigned long expiry = strtoul(licenseExpiry.c_str(), nullptr, 10);
    return millis() <= expiry;
  }

  // Current licenses use the ISO-8601 UTC date sent by the web interface.
  int year, month, day, hour, minute, second;
  if (sscanf(licenseExpiry.c_str(), "%d-%d-%dT%d:%d:%d",
             &year, &month, &day, &hour, &minute, &second) != 6) {
    return false;
  }

  time_t now = time(nullptr);
  // The clock is not available in AP/offline mode. Keep a stored license active
  // until the device can obtain a real wall-clock time.
  if (now < 1700000000) return true;

  struct tm expiryTm = {};
  expiryTm.tm_year = year - 1900;
  expiryTm.tm_mon  = month - 1;
  expiryTm.tm_mday = day;
  expiryTm.tm_hour = hour;
  expiryTm.tm_min  = minute;
  expiryTm.tm_sec  = second;
  return now <= mktime(&expiryTm);
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

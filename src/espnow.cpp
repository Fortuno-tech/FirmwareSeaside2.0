#include "espnow.h"
#include "config.h"
#include "webserver.h"
#include "storage.h"
#include "display.h"
#include "mqtt.h"
#include <esp_now.h>
#include <WiFi.h>

// Structure des paquets ESP-NOW unifiée
typedef struct {
  uint8_t packetType;       // 1 = COUNT, 2 = PAIRING, 3 = CONFIG
  char    moduleId[20];
  int     count;            // count pour COUNT / threshold pour CONFIG
  int     seuil;            // seuil actuel du capteur (rapporté au master)
  char    wifiSSID[32];     // Utilisé pour PAIRING
  char    wifiPassword[64]; // Utilisé pour PAIRING
  char    masterMAC[18];    // Utilisé pour PAIRING
} EspPacket;

static EspPacket s_packetToSend;
static EspPacket s_packetReceived;

// Structure pour suivre les compteurs des Slaves sur le Master
struct SlaveDevice {
  uint8_t mac[6];
  char    moduleId[20];
  int     lastCount;
  bool    active;
  int     seuil;            // Seuil rapporté par l'esclave
};

#define MAX_SLAVES 10
static SlaveDevice s_slaves[MAX_SLAVES] = {};
static int s_slaveCount = 0;

// Callback envoi
void onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
  Serial.print("[ESP-NOW] Envoi au MAC ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? " : Réussi" : " : Échoué");
}

// Accesseurs publics pour le serveur web
int espnow_getSlaveCount() {
  return s_slaveCount;
}

bool espnow_getSlaveInfo(int index, char* outMac, char* outModuleId, int* outCount, bool* outActive, int* outSeuil) {
  if (index < 0 || index >= s_slaveCount) return false;
  snprintf(outMac, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
    s_slaves[index].mac[0], s_slaves[index].mac[1], s_slaves[index].mac[2],
    s_slaves[index].mac[3], s_slaves[index].mac[4], s_slaves[index].mac[5]);
  strncpy(outModuleId, s_slaves[index].moduleId, 20);
  *outCount  = s_slaves[index].lastCount;
  *outActive = s_slaves[index].active;
  *outSeuil  = s_slaves[index].seuil;
  return true;
}

void espnow_resetSlaves() {
  for (int i = 0; i < MAX_SLAVES; i++) {
    s_slaves[i].lastCount = 0;
  }
  Serial.println("→ Compteurs des esclaves réinitialisés sur le Master.");
}

// Callback réception
void onDataReceived(const uint8_t* mac, const uint8_t* data, int len) {
  if (len < (int)sizeof(uint8_t)) return;
  
  memcpy(&s_packetReceived, data, sizeof(s_packetReceived));
  
  // ─── 1. PAQUET PAIRING (Reçu sur Slave non lié) ───
  if (s_packetReceived.packetType == PACKET_TYPE_PAIRING) {
    if (moduleRole != "slave") {
      Serial.println("\n[ESP-NOW] --- RECEPTION D'UN PAQUET DE LIAISON (PAIRING) ---");
      Serial.printf("[ESP-NOW] Master MAC : %s\n", s_packetReceived.masterMAC);
      Serial.printf("[ESP-NOW] SSID WiFi  : %s\n", s_packetReceived.wifiSSID);
      Serial.printf("[ESP-NOW] Module ID  : %s\n", s_packetReceived.moduleId);
      
      masterMAC          = String(s_packetReceived.masterMAC);
      staSSID            = String(s_packetReceived.wifiSSID);
      staPassword        = String(s_packetReceived.wifiPassword);
      moduleId           = String(s_packetReceived.moduleId);
      moduleRole         = "slave";
      isMasterConfigured = true;
      
      storage_saveConfig();
      
      Serial.println("[ESP-NOW] Liaison enregistrée ! Redémarrage dans 2 secondes...");
      requestReboot = true;
      rebootTimer   = millis();
    } else {
      Serial.println("[ESP-NOW] Paquet de liaison reçu mais ce module est déjà configuré comme Slave. Ignoré.");
    }
    return;
  }
  
  // ─── 2. PAQUET CONFIG (Reçu sur Slave pour modifier le seuil) ───
  if (s_packetReceived.packetType == PACKET_TYPE_CONFIG) {
    if (moduleRole == "slave") {
      int val = s_packetReceived.count;
      if (val >= SEUIL_MIN && val <= SEUIL_MAX) {
        seuil = val;
        storage_saveConfig();
        Serial.printf("[ESP-NOW] Seuil mis à jour par le Master : %d cm\n", seuil);
        
        // Renvoyer les données immédiatement pour informer le Master du nouveau seuil
        espnow_sendData((uint8_t*)mac, compteur);
      }
    }
    return;
  }
  
  // ─── 4. PAQUET RESET (Reçu sur Slave pour remettre son compteur à 0) ───
  if (s_packetReceived.packetType == PACKET_TYPE_RESET) {
    if (moduleRole == "slave") {
      compteur = 0;
      storage_markDirty();
      Serial.println("[ESP-NOW] Compteur remis à zéro par le Master");
      triggerImmediateDisplayUpdate();
      // Informer le Master du nouveau compteur
      espnow_sendData((uint8_t*)mac, compteur);
    }
    return;
  }

  // ─── 5. PAQUET FORMAT (Reçu sur Slave pour réinitialiser le module) ───
  if (s_packetReceived.packetType == PACKET_TYPE_FORMAT) {
    if (moduleRole == "slave") {
      Serial.println("[ESP-NOW] Commande de formatage reçue du Master !");
      storage_format();
      delay(500);
      ESP.restart();
    }
    return;
  }

  // ─── 3. PAQUET COUNT (Reçu sur Master depuis Slave) ───
  if (moduleRole == "master" && s_packetReceived.packetType == PACKET_TYPE_COUNT) {
    Serial.print("[ESP-NOW] Données de ");
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X", mac[i]);
      if (i < 5) Serial.print(":");
    }
    Serial.printf(" | ModuleId: %s | Count: %d | Seuil: %d\n", 
      s_packetReceived.moduleId, s_packetReceived.count, s_packetReceived.seuil);

    int slaveIndex = -1;
    for (int i = 0; i < s_slaveCount; i++) {
      if (memcmp(s_slaves[i].mac, mac, 6) == 0) {
        slaveIndex = i;
        break;
      }
    }

    if (slaveIndex == -1 && s_slaveCount < MAX_SLAVES) {
      slaveIndex = s_slaveCount;
      memcpy(s_slaves[slaveIndex].mac, mac, 6);
      s_slaves[slaveIndex].lastCount = 0;
      s_slaves[slaveIndex].active = true;
      s_slaveCount++;
    }

    if (slaveIndex != -1) {
      strncpy(s_slaves[slaveIndex].moduleId, s_packetReceived.moduleId, 20);
      s_slaves[slaveIndex].active = true;
      s_slaves[slaveIndex].seuil  = s_packetReceived.seuil; // Enregistrer le seuil de l'esclave

      int diff = s_packetReceived.count - s_slaves[slaveIndex].lastCount;
      s_slaves[slaveIndex].lastCount = s_packetReceived.count;

      totalPersonnes += diff;
      personnesActuelles += diff;

      if (totalPersonnes < 0) totalPersonnes = 0;
      if (personnesActuelles < 0) personnesActuelles = 0;

      Serial.printf("→ Slave [%s] Diff: %+d | Total Global = %d | Seuil: %d cm\n",
        s_packetReceived.moduleId, diff, totalPersonnes, s_packetReceived.seuil);

      triggerImmediateDisplayUpdate();
      webserver_broadcastCount(totalPersonnes);

      // Publier les entrées individuelles si diff > 0
      if (diff > 0) {
        for (int i = 0; i < diff; i++) {
          mqttPublishEntry();
        }
      }

      // Publier la télémétrie de l'esclave
      char macStr[18];
      snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      mqttPublishSlaveTelemetry(s_packetReceived.moduleId, macStr, s_packetReceived.count, s_packetReceived.seuil, true);
    }
  }
}

// Initialisation commune d'ESP-NOW
static void initESPNOW_Common() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("✗ Erreur: initialisation ESP-NOW échouée");
    return;
  }
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataReceived);
  Serial.println("✓ ESP-NOW Initialisé (Send & Recv enregistrés)");
}

void setupESPNOW_Master() {
  initESPNOW_Common();
  Serial.println("→ Mode Master actif sur ESP-NOW");
}

void setupESPNOW_Slave() {
  initESPNOW_Common();
  Serial.println("→ Mode Slave actif sur ESP-NOW");
}

void espnow_addSlave(uint8_t* mac) {
  if (esp_now_is_peer_exist(mac)) return;
  
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("✗ Erreur: échec de l'ajout du pair (peer)");
  } else {
    Serial.println("✓ Pair ajouté avec succès");
  }
}

void espnow_sendData(uint8_t* mac, int count) {
  s_packetToSend.packetType = PACKET_TYPE_COUNT;
  strncpy(s_packetToSend.moduleId, moduleId.c_str(), 20);
  s_packetToSend.count = count;
  s_packetToSend.seuil = seuil; // On transmet notre seuil actuel
  
  esp_now_send(mac, (uint8_t*)&s_packetToSend, sizeof(s_packetToSend));
}

void espnow_sendPairing(uint8_t* targetMac, const char* name, const char* ssid, const char* password, const char* masterMacStr) {
  espnow_addSlave(targetMac);
  
  s_packetToSend.packetType = PACKET_TYPE_PAIRING;
  strncpy(s_packetToSend.moduleId, name, 20);
  s_packetToSend.count = 0;
  s_packetToSend.seuil = seuil;
  strncpy(s_packetToSend.wifiSSID, ssid, 32);
  strncpy(s_packetToSend.wifiPassword, password, 64);
  strncpy(s_packetToSend.masterMAC, masterMacStr, 18);
  
  Serial.printf("[ESP-NOW] Envoi du paquet de pairing au MAC esclave...\n");
  esp_now_send(targetMac, (uint8_t*)&s_packetToSend, sizeof(s_packetToSend));
}

void espnow_sendConfig(uint8_t* targetMac, int threshold) {
  espnow_addSlave(targetMac);
  
  s_packetToSend.packetType = PACKET_TYPE_CONFIG;
  s_packetToSend.count = threshold;
  s_packetToSend.seuil = threshold;
  
  Serial.printf("[ESP-NOW] Envoi de la config de seuil (%d cm) au MAC...\n", threshold);
  esp_now_send(targetMac, (uint8_t*)&s_packetToSend, sizeof(s_packetToSend));
}

void espnow_sendReset(uint8_t* targetMac) {
  espnow_addSlave(targetMac);
  s_packetToSend.packetType = PACKET_TYPE_RESET;
  s_packetToSend.count = 0;
  Serial.printf("[ESP-NOW] Envoi de la commande RESET au MAC...\n");
  esp_now_send(targetMac, (uint8_t*)&s_packetToSend, sizeof(s_packetToSend));
}

void espnow_sendFormat(uint8_t* targetMac) {
  espnow_addSlave(targetMac);
  s_packetToSend.packetType = PACKET_TYPE_FORMAT;
  Serial.printf("[ESP-NOW] Envoi de la commande FORMAT au MAC...\n");
  esp_now_send(targetMac, (uint8_t*)&s_packetToSend, sizeof(s_packetToSend));
}

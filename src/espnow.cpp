#include "espnow.h"
#include "config.h"
#include "webserver.h"
#include <esp_now.h>
#include <WiFi.h>
// Structure des données échangées
typedef struct {
  char moduleId[10];
  int count;
} DataPacket;

DataPacket dataToSend;
DataPacket dataReceived;

// Callback envoi (Slave)
void onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
  Serial.print("Envoi : ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAILED");
}

// Structure pour suivre les compteurs des Slaves
struct SlaveDevice {
  uint8_t mac[6];
  int lastCount;
  bool active;
};

#define MAX_SLAVES 10
static SlaveDevice s_slaves[MAX_SLAVES] = {0};
static int s_slaveCount = 0;

void espnow_resetSlaves() {
  for (int i = 0; i < MAX_SLAVES; i++) {
    s_slaves[i].lastCount = 0;
  }
  Serial.println("→ Compteurs des esclaves réinitialisés sur le Master.");
}

// Callback réception (Master)
void onDataReceived(const uint8_t* mac, const uint8_t* data, int len) {
  memcpy(&dataReceived, data, sizeof(dataReceived));
  Serial.print("Reçu de : ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.print(" | Count : ");
  Serial.println(dataReceived.count);

  // Rechercher ou ajouter l'esclave dans notre liste
  int slaveIndex = -1;
  for (int i = 0; i < s_slaveCount; i++) {
    if (memcmp(s_slaves[i].mac, mac, 6) == 0) {
      slaveIndex = i;
      break;
    }
  }

  if (slaveIndex == -1 && s_slaveCount < MAX_SLAVES) {
    // Ajouter un nouvel esclave
    slaveIndex = s_slaveCount;
    memcpy(s_slaves[slaveIndex].mac, mac, 6);
    s_slaves[slaveIndex].lastCount = 0; // On suppose qu'il commence à 0
    s_slaves[slaveIndex].active = true;
    s_slaveCount++;
  }

  if (slaveIndex != -1) {
    int diff = dataReceived.count - s_slaves[slaveIndex].lastCount;
    s_slaves[slaveIndex].lastCount = dataReceived.count;

    totalPersonnes += diff;
    personnesActuelles += diff;

    if (totalPersonnes < 0) totalPersonnes = 0;
    if (personnesActuelles < 0) personnesActuelles = 0;

    Serial.printf("→ Slave Diff: %+d | Total Global = %d\n", diff, totalPersonnes);

    // Diffusion du nouveau total
    webserver_broadcastCount(totalPersonnes);
  }
}

// Setup Master
void setupESPNOW_Master() {
  WiFi.mode(WIFI_AP_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erreur ESP-NOW init !");
    return;
  }
  esp_now_register_recv_cb(onDataReceived);
  Serial.println("ESP-NOW Master prêt !");
  Serial.print("MAC Master : ");
  Serial.println(WiFi.macAddress());
}

// Setup Slave
void setupESPNOW_Slave() {
  WiFi.mode(WIFI_AP_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erreur ESP-NOW init !");
    return;
  }
  esp_now_register_send_cb(onDataSent);
  Serial.println("ESP-NOW Slave pret !");
  Serial.print("MAC Slave : ");
  Serial.println(WiFi.macAddress());
}

// Ajouter un Slave (depuis Master)
void espnow_addSlave(uint8_t* mac) {
  if (esp_now_is_peer_exist(mac)) {
    return; // Déjà ajouté
  }
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Erreur ajout peer !");
  } else {
    Serial.println("Slave ajouté !");
  }
}

// Envoyer données (depuis Slave)
void espnow_sendData(uint8_t* mac, int count) {
  strcpy(dataToSend.moduleId, "slave1");
  dataToSend.count = count;
  esp_now_send(mac, (uint8_t*)&dataToSend, sizeof(dataToSend));
}



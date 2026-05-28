#include "espnow.h"
#include "config.h"

#ifdef ESP32

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

  // Mise à jour compteur global
  totalPersonnes     = dataReceived.count;
  personnesActuelles = dataReceived.count;
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
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erreur ESP-NOW init !");
    return;
  }
  esp_now_register_send_cb(onDataSent);
  Serial.println("ESP-NOW Slave prêt !");
  Serial.print("MAC Slave : ");
  Serial.println(WiFi.macAddress());
}

// Ajouter un Slave (depuis Master)
void espnow_addSlave(uint8_t* mac) {
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

#endif
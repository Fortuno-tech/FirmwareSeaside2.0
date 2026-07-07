#ifndef ESPNOW_H
#define ESPNOW_H

#include <Arduino.h>

#define PACKET_TYPE_COUNT   1
#define PACKET_TYPE_PAIRING 2
#define PACKET_TYPE_CONFIG  3
#define PACKET_TYPE_RESET   4
#define PACKET_TYPE_FORMAT  5

void setupESPNOW_Master();
void setupESPNOW_Slave();
void espnow_addSlave(uint8_t* mac);
void espnow_sendData(uint8_t* mac, int count);
void espnow_sendPairing(uint8_t* targetMac, const char* name, const char* ssid, const char* password, const char* masterMacStr);
void espnow_sendConfig(uint8_t* targetMac, int threshold);
void espnow_sendReset(uint8_t* targetMac);
void espnow_sendFormat(uint8_t* targetMac);
void espnow_resetSlaves();

// Accesseurs pour les données des slaves (utilisés par le webserver)
int  espnow_getSlaveCount();
bool espnow_getSlaveInfo(int index, char* outMac, char* outModuleId, int* outCount, bool* outActive, int* outSeuil);

#endif

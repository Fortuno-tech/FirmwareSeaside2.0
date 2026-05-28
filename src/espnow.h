#ifndef ESPNOW_H
#define ESPNOW_H

#include <Arduino.h>

#ifdef ESP32

void setupESPNOW_Master();
void setupESPNOW_Slave();
void espnow_addSlave(uint8_t* mac);
void espnow_sendData(uint8_t* mac, int count);

#endif

#endif
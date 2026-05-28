#ifndef UDP_H
#define UDP_H

#include <Arduino.h>

#ifndef ESP32

void setupUDP_Master();
void setupUDP_Slave();
void udp_sendData(const char* masterIP, int count);
void udp_receive();

#endif

#endif
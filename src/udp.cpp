#include "udp.h"
#include "config.h"

#ifndef ESP32

// Simulation UDP via Serial pour ESP8266
// La vraie communication se fera via ESP32 en production

void setupUDP_Master() {
  Serial.println("UDP Master prêt (simulation ESP8266) !");
}

void setupUDP_Slave() {
  Serial.println("UDP Slave prêt (simulation ESP8266) !");
}

void udp_sendData(const char* masterIP, int count) {
  Serial.print("UDP simulé envoi count : ");
  Serial.println(count);
  totalPersonnes     = count;
  personnesActuelles = count;
}

void udp_receive() {
  // Simulation : rien à faire sur ESP8266
}

#endif
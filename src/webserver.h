#ifndef PROJECT_WEBSERVER_H
#define PROJECT_WEBSERVER_H

#include <ESPAsyncWebServer.h>

extern AsyncWebServer server;
extern AsyncWebSocket ws;

void setupServer();
void webserver_broadcastCount(int val);

#endif // PROJECT_WEBSERVER_H


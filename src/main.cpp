#include <Arduino.h>
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266Wifi.h>
#endif

const char* AP_SSID ="SmartCount";
const char* AP_PASSWORD= "Fortico1234";
void setup(){
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(AP_SSID, AP_PASSWORD);

  if (ok){
    Serial.println("AP démarré");
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());
  } else{
    Serial.println("Echec");
  }
}
void loop(){
  
}
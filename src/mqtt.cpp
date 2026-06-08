#include "mqtt.h"
#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Config MQTT
const char* MQTT_SERVER = "192.168.1.2";
const int   MQTT_PORT   = 1883;
const char* MQTT_USER   = "";
const char* MQTT_PASS   = "";

// Topics
const char* TOPIC_COUNT  = "seaside/count";
const char* TOPIC_CONFIG = "seaside/config";
const char* TOPIC_STATUS = "seaside/status";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Callback réception messages MQTT
void onMQTTMessage(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("MQTT reçu [");
  Serial.print(topic);
  Serial.print("] : ");
  Serial.println(msg);

  // Commande config reçue
  if (String(topic) == TOPIC_CONFIG) {
    StaticJsonDocument<200> doc;
    deserializeJson(doc, msg);
  }
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connexion MQTT...");
    if (mqttClient.connect("Seaside2-Master", MQTT_USER, MQTT_PASS)) {
      Serial.println("connecté !");
      mqttClient.subscribe(TOPIC_CONFIG);
      mqttClient.subscribe(TOPIC_STATUS);
    } else {
      Serial.print("Échec, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" → retry dans 5s");
      delay(5000);
    }
  }
}

void setupMQTT(const char* ssid, const char* password) {
  // Connexion WiFi STA
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connexion WiFi");
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connecté !");
    Serial.print("IP STA : ");
    Serial.println(WiFi.localIP());

    // Setup MQTT
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(onMQTTMessage);
    connectMQTT();
  } else {
    Serial.println("\nWiFi non disponible — mode offline");
  }
}

void handleMQTT() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      connectMQTT();
    }
    mqttClient.loop();
  }
}

void mqttPublishCount(int total, int current) {
  if (mqttClient.connected()) {
    StaticJsonDocument<200> doc;
    doc["total"]   = total;
    doc["current"] = current;
    doc["module"]  = "master";

    String msg;
    serializeJson(doc, msg);
    mqttClient.publish(TOPIC_COUNT, msg.c_str());
    Serial.print("MQTT publié : ");
    Serial.println(msg);
  }
}
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

static unsigned long lastMqttRetry = 0;
static unsigned long lastWifiCheck = 0;
static bool wasWifiConnected = false;

bool connectMQTTNonBlocking() {
  Serial.print("Essai de connexion MQTT vers ");
  Serial.print(mqttServer);
  Serial.print(":");
  Serial.print(mqttPort);
  Serial.println("...");

  // Génère un ID client unique basé sur l'adresse MAC
  String clientId = "Seaside2-Master-" + WiFi.macAddress();
  clientId.replace(":", "");

  bool connected = false;
  if (mqttUser.length() > 0) {
    Serial.print("Utilisation de l'utilisateur : ");
    Serial.println(mqttUser);
    connected = mqttClient.connect(clientId.c_str(), mqttUser.c_str(), mqttPassword.c_str());
  } else {
    Serial.println("Connexion anonyme (sans identifiants).");
    connected = mqttClient.connect(clientId.c_str());
  }

  if (connected) {
    Serial.println("✓ MQTT connecté !");
    mqttClient.subscribe(TOPIC_CONFIG);
    mqttClient.subscribe(TOPIC_STATUS);
    return true;
  } else {
    Serial.print("✗ Échec connexion MQTT, rc=");
    Serial.println(mqttClient.state());
    return false;
  }
}

void setupMQTT(const char* ssid, const char* password) {
  // Mode AP+STA pour que le point d'accès local fonctionne toujours
  WiFi.mode(WIFI_AP_STA);
  
  if (ssid != nullptr && strlen(ssid) > 0) {
    Serial.print("WiFi STA : Tentative de connexion à ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
  }

  // Configuration de MQTT en utilisant les variables globales
  if (mqttClient.connected()) {
    mqttClient.disconnect();
  }
  mqttClient.setServer(mqttServer.c_str(), mqttPort);
  mqttClient.setCallback(onMQTTMessage);

  // Réinitialiser les états
  lastMqttRetry = 0;
  lastWifiCheck = 0;
  wasWifiConnected = false;
}

void handleMQTT() {
  unsigned long now = millis();

  if (WiFi.status() == WL_CONNECTED) {
    if (!wasWifiConnected) {
      Serial.println("\n✓ WiFi STA connecté !");
      Serial.print("IP STA : ");
      Serial.println(WiFi.localIP());
      wasWifiConnected = true;
    }

    if (!mqttClient.connected()) {
      if (now - lastMqttRetry >= 5000 || lastMqttRetry == 0) {
        lastMqttRetry = now;
        connectMQTTNonBlocking();
      }
    } else {
      mqttClient.loop();
    }
  } else {
    if (wasWifiConnected) {
      Serial.println("✗ WiFi STA déconnecté !");
      wasWifiConnected = false;
      lastMqttRetry = 0;
    }
    
    // Tentative de reconnexion périodique (toutes les 30 secondes)
    if (staSSID != "" && (now - lastWifiCheck >= 30000 || lastWifiCheck == 0)) {
      lastWifiCheck = now;
      Serial.println("En attente de connexion WiFi STA. Tentative de reconnexion...");
      WiFi.begin(staSSID.c_str(), staPassword.c_str());
    }
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
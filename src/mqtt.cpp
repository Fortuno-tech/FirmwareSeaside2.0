#include "mqtt.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "storage.h"

// Config MQTT local fallbacks
const char* MQTT_SERVER = "53cc1d1dc297463f9f511baf26ee908e.s1.eu.hivemq.cloud";
const int   MQTT_PORT   = 8883;
const char* MQTT_USER   = "Fortico";
const char* MQTT_PASS   = "Fortico123456";

// Topics
const char* TOPIC_ALERTE = "seaside/alerte";

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

// Callback réception messages MQTT
void onMQTTMessage(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("MQTT reçu [");
  Serial.print(topic);
  Serial.print("] : ");
  Serial.println(msg);

  // Vérifier si le topic est seaside/config/{macAddress}
  String topicStr = String(topic);
  String myMac = WiFi.macAddress();
  String myMacNoColons = myMac;
  myMacNoColons.replace(":", "");

  if (topicStr.endsWith(myMac) || topicStr.endsWith(myMacNoColons)) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, msg);
    if (!error) {
      if (doc.containsKey("seuil")) {
        int val = doc["seuil"].as<int>();
        if (val >= SEUIL_MIN && val <= SEUIL_MAX) {
          seuil = val;
          storage_saveConfig();
          Serial.printf("[MQTT] Seuil mis à jour à %d cm depuis MQTT\n", seuil);
        } else {
          mqttPublishAlert("Erreur configuration: seuil hors limites (" + String(val) + " cm)");
        }
      }
    } else {
      Serial.println("[MQTT] Erreur deserialisation JSON config");
    }
  }
}

static unsigned long lastMqttRetry = 0;
static unsigned long lastWifiCheck = 0;
static bool wasWifiConnected = false;

bool connectMQTTNonBlocking() {
  Serial.print("Essai de connexion MQTT SSL vers ");
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
    
    // Souscrire à seaside/config/{macAddress}
    String topicConfig = "seaside/config/" + WiFi.macAddress();
    mqttClient.subscribe(topicConfig.c_str());
    Serial.printf("[MQTT] Souscrit à %s\n", topicConfig.c_str());
    
    String topicConfigNoColons = "seaside/config/" + WiFi.macAddress();
    topicConfigNoColons.replace(":", "");
    mqttClient.subscribe(topicConfigNoColons.c_str());
    Serial.printf("[MQTT] Souscrit à %s\n", topicConfigNoColons.c_str());
    
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

  // Activer le mode non sécurisé de WiFiClientSecure pour ne pas valider le certificat de façon stricte (évite de stocker un certificat racine qui expire)
  espClient.setInsecure();

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

#include "webserver.h"

void mqttPublishCount(int total, int current) {
  if (mqttClient.connected()) {
    String msg = webserver_getMqttPayloadJson();
    mqttClient.publish("seaside/telemetry/master", msg.c_str());
    Serial.print("MQTT publié (seaside/telemetry/master) : ");
    Serial.println(msg);
  }
}

void mqttPublishEntry() {
  if (mqttClient.connected()) {
    String categoryId = (licenseCode.length() > 0) ? licenseCode : "default";
    String topic = "seaside/entrees/" + categoryId;
    
    StaticJsonDocument<256> doc;
    doc["mac"] = WiFi.macAddress();
    doc["moduleId"] = moduleId;
    doc["timestamp"] = millis();
    
    String msg;
    serializeJson(doc, msg);
    mqttClient.publish(topic.c_str(), msg.c_str());
    Serial.printf("MQTT entrée publiée (%s) : %s\n", topic.c_str(), msg.c_str());
  }
}

void mqttPublishSlaveTelemetry(const char* slaveId, const char* mac, int count, int seuil, bool active) {
  if (mqttClient.connected()) {
    String topic = "seaside/telemetry/slave/" + String(slaveId);
    
    StaticJsonDocument<256> doc;
    doc["slaveId"] = slaveId;
    doc["mac"] = mac;
    doc["count"] = count;
    doc["seuil"] = seuil;
    doc["active"] = active;
    
    String msg;
    serializeJson(doc, msg);
    mqttClient.publish(topic.c_str(), msg.c_str());
    Serial.printf("MQTT télémétrie esclave publiée (%s) : %s\n", topic.c_str(), msg.c_str());
  }
}

void mqttPublishAlert(const String& message) {
  if (mqttClient.connected()) {
    StaticJsonDocument<256> doc;
    doc["mac"] = WiFi.macAddress();
    doc["moduleId"] = moduleId;
    doc["alerte"] = message;
    
    String msg;
    serializeJson(doc, msg);
    mqttClient.publish(TOPIC_ALERTE, msg.c_str());
    Serial.printf("MQTT alerte publiée : %s\n", msg.c_str());
  }
}
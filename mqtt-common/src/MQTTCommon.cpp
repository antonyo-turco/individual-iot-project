#include "MQTTCommon.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>

static WiFiClient plainClient;
static WiFiClientSecure secureClient;
PubSubClient mqttClient;

static const char* _ssid = nullptr;
static const char* _password = nullptr;
static const char* _clientId = nullptr;
static StatusCallback _statusCb = nullptr;

static void statusMsg(const char* msg) {
  if (_statusCb) _statusCb(msg);
}

static void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  statusMsg("[WiFi] Connecting...");
  Serial.print("[WiFi] Connecting to ");
  Serial.println(_ssid);
  WiFi.begin(_ssid, _password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\n[WiFi] Connected, IP: ");
    Serial.println(WiFi.localIP());
    statusMsg("WiFi OK");
    delay(1000);

    statusMsg("Syncing time...");
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    int waited = 0;
    while (!getLocalTime(&timeinfo) && waited < 10) {
      delay(500);
      waited++;
    }
    if (waited < 10) {
      Serial.printf("[NTP] Time synced: %04d-%02d-%02d\n",
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    } else {
      Serial.println("[NTP] Time sync failed — TLS cert validation may fail.");
    }
  } else {
    Serial.println("\n[WiFi] Failed to connect.");
    statusMsg("WiFi FAIL");
  }
}

static void connectMQTT() {
  if (mqttClient.connected()) return;

  statusMsg("[MQTT] Connecting...");
  Serial.print("[MQTT] Connecting to broker...");
  int attempts = 0;
  while (!mqttClient.connected() && attempts < 5) {
    if (mqttClient.connect(_clientId)) {
      Serial.println(" OK.");
      statusMsg("MQTT OK");
      delay(1000);
    } else {
      Serial.print(" failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Retrying...");
      delay(2000);
      attempts++;
    }
  }
}

void mqttCommonInit(const char* ssid, const char* password,
                    const char* broker, int port, const char* clientId,
                    const char* caCert,
                    StatusCallback statusCb) {
  _ssid = ssid;
  _password = password;
  _clientId = clientId;
  _statusCb = statusCb;

  connectWiFi();

  if (caCert != nullptr) {
    secureClient.setCACert(caCert);
    mqttClient.setClient(secureClient);
    Serial.println("[MQTT] TLS enabled with CA certificate.");
  } else {
    mqttClient.setClient(plainClient);
    Serial.println("[MQTT] TLS disabled, using plain TCP.");
  }

  mqttClient.setServer(broker, port);
  connectMQTT();
}

void mqttCommonSetCallback(MQTT_CALLBACK_SIGNATURE) {
  mqttClient.setCallback(callback);
}

void mqttCommonSubscribe(const char* topic) {
  if (mqttClient.connected()) {
    mqttClient.subscribe(topic);
    Serial.print("[MQTT] Subscribed to ");
    Serial.println(topic);
  }
}

bool mqttCommonPublish(const char* topic, const char* payload) {
  connectWiFi();
  connectMQTT();
  mqttClient.loop();

  if (mqttClient.publish(topic, payload)) {
    Serial.print("[MQTT] Published to ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(payload);
    return true;
  }
  Serial.println("[MQTT] Publish failed.");
  return false;
}

void mqttCommonLoop() {
  if (mqttClient.connected()) {
    mqttClient.loop();
  }
}

bool mqttCommonConnected() {
  return mqttClient.connected();
}

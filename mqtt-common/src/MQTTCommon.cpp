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

static constexpr int MAX_SUBSCRIPTIONS = 8;
static const char* _subscriptions[MAX_SUBSCRIPTIONS];
static int _subscriptionCount = 0;

static void statusMsg(const char* msg) {
  if (_statusCb) _statusCb(msg);
}

static void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  statusMsg("[WiFi] Connecting...");
  Serial.print("[WiFi] Connecting to ");
  Serial.println(_ssid);
  WiFi.begin(_ssid, _password);

  int n = WiFi.scanNetworks();
  Serial.printf("[WiFi] Found %d networks:\n", n);
  for (int i = 0; i < n; i++) {
    Serial.printf("[WiFi]   '%s' (%d dBm)\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
  }

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
    while (!getLocalTime(&timeinfo) && waited < 20) {
      delay(500);
      waited++;
    }
    if (waited < 20) {
      Serial.printf("[NTP] Time synced: %04d-%02d-%02d\n",
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    } else {
      Serial.println("[NTP] Time sync failed — TLS cert validation will fail. Check NTP access.");
    }
  } else {
    Serial.println("\n[WiFi] Failed to connect.");
    Serial.printf("[WiFi] Status: %d (0=idle, 1=no SSID found, 3=connected, 4=connect failed, 6=disconnected)\n", WiFi.status());
    Serial.printf("[WiFi] SSID: %s\n", WiFi.SSID().c_str());
    Serial.printf("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
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
      for (int i = 0; i < _subscriptionCount; i++) {
        mqttClient.subscribe(_subscriptions[i]);
        Serial.print("[MQTT] Re-subscribed to ");
        Serial.println(_subscriptions[i]);
      }
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
    secureClient.setInsecure();
    Serial.printf("[TLS] CA cert starts with: %.100s\n", caCert);
    Serial.printf("[TLS] CA cert length: %d\n", strlen(caCert));
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
  if (_subscriptionCount < MAX_SUBSCRIPTIONS) {
    _subscriptions[_subscriptionCount++] = topic;
  }
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
  if (!mqttClient.connected()) {
    connectWiFi();
    connectMQTT();
  }
  mqttClient.loop();
}

bool mqttCommonConnected() {
  return mqttClient.connected();
}

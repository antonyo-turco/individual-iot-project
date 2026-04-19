#include "MQTTClient.h"
#include "Secrets.h"
#include "OLEDDisplay.h"
#include "BoardConfig.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define MQTT_TOPIC_CMD "iot/sensor/command"

static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);
static float* pCurrentSampleRate = nullptr;

static void onMessage(char* topic, byte* payload, unsigned int length) {
  char msg[length + 1];
  memcpy(msg, payload, length);
  msg[length] = '\0';

  Serial.print("[MQTT] Command received: ");
  Serial.println(msg);
  showBriefMessage(msg);
  delay(800);

  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, msg) != DeserializationError::Ok) {
    Serial.println("[MQTT] Failed to parse JSON.");
    return;
  }

  const char* cmd = doc["cmd"];
  if (cmd == nullptr) {
    Serial.println("[MQTT] No 'cmd' field.");
    return;
  }

  if (strcmp(cmd, "set_sample_rate") == 0) {
    float rate = doc["value"];
    if (pCurrentSampleRate != nullptr) {
      *pCurrentSampleRate = constrain(rate, 10.0f, SAMPLE_RATE_HZ);
      Serial.printf("[MQTT] Sample rate set to %.1f Hz\n", *pCurrentSampleRate);
    }
  } else if (strcmp(cmd, "reset") == 0) {
    Serial.println("[MQTT] Restarting...");
    ESP.restart();
  }
}

static void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  showConnectionStatus("[WiFi] Connecting...");
  Serial.print("[WiFi] Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\n[WiFi] Connected, IP: ");
    Serial.println(WiFi.localIP());
    showConnectionStatus("WiFi OK");
    delay(1000);
  } else {
    Serial.println("\n[WiFi] Failed to connect.");
    showConnectionStatus("WiFi FAIL");
  }
}

static void connectMQTT() {
  if (mqttClient.connected()) return;

  showConnectionStatus("[MQTT] Connecting...");
  Serial.print("[MQTT] Connecting to broker...");
  int attempts = 0;
  while (!mqttClient.connected() && attempts < 5) {
    if (mqttClient.connect("heltec-v3")) {
      Serial.println(" OK.");
      mqttClient.subscribe(MQTT_TOPIC_CMD);
      Serial.print("[MQTT] Subscribed to ");
      Serial.println(MQTT_TOPIC_CMD);
      showConnectionStatus("MQTT OK");
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

void initMQTT() {
  connectWiFi();
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(onMessage);
  connectMQTT();
}

void publishAggregate(float avg, float dominantHz, float sampleRateHz, uint32_t windowMs) {
  connectWiFi();
  connectMQTT();
  mqttClient.loop();

  char payload[128];
  snprintf(payload, sizeof(payload),
    "{\"avg\":%.3f,\"dominant_hz\":%.2f,\"sample_rate_hz\":%.2f,\"window_ms\":%lu,\"timestamp\":%lu}",
    avg, dominantHz, sampleRateHz, windowMs, millis()
  );

  if (mqttClient.publish(MQTT_TOPIC, payload)) {
    Serial.print("[MQTT] Published: ");
    Serial.println(payload);
  } else {
    Serial.println("[MQTT] Publish failed.");
  }
}

void setSampleRatePtr(float* ptr) {
  pCurrentSampleRate = ptr;
}

void processMQTT() {
  if (mqttClient.connected()) {
    mqttClient.loop();
  }
}

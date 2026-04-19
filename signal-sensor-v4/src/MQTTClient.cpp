#include "MQTTClient.h"
#include "Secrets.h"
#include "OLEDDisplay.h"
#include <WiFi.h>
#include <PubSubClient.h>

static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);

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

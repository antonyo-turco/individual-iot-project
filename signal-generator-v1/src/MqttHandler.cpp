#include <Arduino.h>
#include "MqttHandler.h"
#include "SignalManager.h"
#include "Secrets.h"
#include <MQTTCommon.h>
#include <ArduinoJson.h>

#define MQTT_TOPIC_CMD "iot/generator/command"

void MqttHandleMessage(char* topic, byte* payload, unsigned int length) {
  char msg[length + 1];
  memcpy(msg, payload, length);
  msg[length] = '\0';

  Serial.print("[MqttHandler] Command received: ");
  Serial.println(msg);

  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, msg) != DeserializationError::Ok) {
    Serial.println("[MqttHandler] Failed to parse JSON.");
    return;
  }

  const char* cmd = doc["cmd"];
  if (cmd == nullptr) {
    Serial.println("[MqttHandler] No 'cmd' field.");
    return;
  }

  if (strcmp(cmd, "start") == 0) {
    SignalManagerStartSignal();
  } else if (strcmp(cmd, "stop") == 0) {
    SignalManagerStopSignal();
  } else if (strcmp(cmd, "set_signal") == 0) {
    const char* name = doc["value"];
    if (name != nullptr) {
      SignalManagerSetSignal(name);
    } else {
      Serial.println("[MqttHandler] set_signal: missing 'value' field.");
    }
  } else if (strcmp(cmd, "set_noise") == 0) {
    bool enabled = doc["enabled"] | false;
    SignalManagerSetNoiseEnabled(enabled);
  } else if (strcmp(cmd, "set_noise_params") == 0) {
    NoiseParams current = SignalManagerGetNoiseParams();
    float sigma = doc["sigma"] | current.sigma;
    float spikeProb = doc["spike_prob"] | current.spikeProb;
    float spikeMin = doc["spike_min"] | current.spikeMin;
    float spikeMax = doc["spike_max"] | current.spikeMax;
    SignalManagerSetNoiseParams(sigma, spikeProb, spikeMin, spikeMax);
  } else if (strcmp(cmd, "set_custom_signal") == 0) {
    JsonArray harmonics = doc["harmonics"];
    if (harmonics.isNull()) {
      Serial.println("[MqttHandler] set_custom_signal: missing 'harmonics' array.");
      return;
    }
    CustomHarmonic customHarmonics_tmp[MAX_HARMONICS];
    int count = 0;
    for (JsonObject h : harmonics) {
      if (count >= MAX_HARMONICS) break;
      customHarmonics_tmp[count].amplitude = h["a"];
      customHarmonics_tmp[count].frequency = h["f"];
      count++;
    }
    SignalManagerSetCustomSignal(customHarmonics_tmp, count);
  } else {
    Serial.printf("[MqttHandler] Unknown command: %s\n", cmd);
  }
}

void MqttInit() {
  mqttCommonInit(WIFI_SSID, WIFI_PASSWORD, MQTT_BROKER, MQTT_PORT, "esp32-generator", CA_CERT);
  mqttCommonSetCallback(MqttHandleMessage);
  mqttCommonSubscribe(MQTT_TOPIC_CMD);
  Serial.println("[MqttHandler] Initialized");
}

void MqttLoop() {
  mqttCommonLoop();
}

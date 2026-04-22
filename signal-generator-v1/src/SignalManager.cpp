#include <Arduino.h>
#include "SignalManager.h"
#include "Waveform.h"
#include "Secrets.h"
#include <MQTTCommon.h>
#include <ArduinoJson.h>
#include <esp_random.h>
#include <cmath>

#define MQTT_TOPIC_SIGNAL "iot/generator/signal"

static SignalType currentSignal = COMPOSITE;
static bool signalRunning = true;
static bool noiseEnabled = false;
static float freqMultiplier = 1.0f;
static NoiseParams noiseParams;

static float gaussianNoise(float sigma) {
  float u1 = (esp_random() + 1.0f) / (UINT32_MAX + 2.0f);
  float u2 = esp_random() / (UINT32_MAX + 1.0f);
  return sigma * sqrtf(-2.0f * logf(u1)) * cosf(2.0f * PI * u2);
}

void SignalManagerInit() {
  // Signal manager ready
}

void SignalManagerNextSignal() {
  currentSignal = nextSignal(currentSignal);
  Serial.printf("[SignalManager] Signal: %s\n", SIGNAL_NAMES[currentSignal]);
  SignalManagerPublishState();
}

void SignalManagerSetSignal(const char* name) {
  for (int i = 0; i < SIGNAL_COUNT; i++) {
    if (strcasecmp(name, SIGNAL_NAMES[i]) == 0) {
      currentSignal = (SignalType)i;
      Serial.printf("[SignalManager] Signal set to %s\n", SIGNAL_NAMES[i]);
      SignalManagerPublishState();
      return;
    }
  }
  Serial.printf("[SignalManager] Unknown signal: %s\n", name);
}

void SignalManagerStartSignal() {
  signalRunning = true;
  Serial.println("[SignalManager] Signal started.");
  SignalManagerPublishState();
}

void SignalManagerStopSignal() {
  signalRunning = false;
  dacWrite(DAC_PIN, 0);
  Serial.println("[SignalManager] Signal stopped.");
  SignalManagerPublishState();
}

void SignalManagerSetNoiseEnabled(bool enabled) {
  noiseEnabled = enabled;
  Serial.printf("[SignalManager] Noise %s\n", enabled ? "enabled" : "disabled");
  SignalManagerPublishState();
}

void SignalManagerSetNoiseParams(float sigma, float spikeProb, float spikeMin, float spikeMax) {
  noiseParams.sigma = sigma;
  noiseParams.spikeProb = spikeProb;
  noiseParams.spikeMin = spikeMin;
  noiseParams.spikeMax = spikeMax;
  Serial.printf("[SignalManager] Noise params: sigma=%.3f prob=%.3f min=%.2f max=%.2f\n",
    sigma, spikeProb, spikeMin, spikeMax);
  if (noiseEnabled) SignalManagerPublishState();
}

void SignalManagerSetCustomSignal(CustomHarmonic* harmonics, int count) {
  int n = (count > MAX_HARMONICS) ? MAX_HARMONICS : count;
  for (int i = 0; i < n; i++) {
    customHarmonics[i] = harmonics[i];
  }
  customHarmonicCount = n;
  Serial.printf("[SignalManager] Custom signal set with %d harmonic(s).\n", n);
  if (currentSignal == CUSTOM) SignalManagerPublishState();
}

SignalType SignalManagerGetCurrentSignal() {
  return currentSignal;
}

bool SignalManagerIsRunning() {
  return signalRunning;
}

bool SignalManagerIsNoiseEnabled() {
  return noiseEnabled;
}

NoiseParams SignalManagerGetNoiseParams() {
  return noiseParams;
}

void SignalManagerSetFreqMultiplier(float mult) {
  freqMultiplier = mult;
  Serial.printf("[SignalManager] Freq multiplier set to %.2fx\n", mult);
  SignalManagerPublishState();
}

float SignalManagerGetFreqMultiplier() {
  return freqMultiplier;
}

float SignalManagerComputeSignal(float t) {
  float signal = computeSignal(currentSignal, t, freqMultiplier);

  if (noiseEnabled) {
    signal += gaussianNoise(noiseParams.sigma) * SCALE_FACTOR;

    float roll = esp_random() / (UINT32_MAX + 1.0f);
    if (roll < noiseParams.spikeProb) {
      float range = noiseParams.spikeMax - noiseParams.spikeMin;
      float mag = noiseParams.spikeMin + (esp_random() / (UINT32_MAX + 1.0f)) * range;
      signal += (esp_random() & 1) ? mag * SCALE_FACTOR : -mag * SCALE_FACTOR;
    }
  }

  return signal;
}

void SignalManagerPublishState() {
  if (currentSignal == CUSTOM) {
    StaticJsonDocument<512> doc;
    doc["signal"] = SIGNAL_NAMES[currentSignal];
    doc["running"] = signalRunning;
    doc["freq_mult"] = freqMultiplier;
    doc["timestamp"] = millis();
    if (noiseEnabled) {
      doc["noise_enabled"] = true;
      doc["sigma"] = noiseParams.sigma;
      doc["spike_prob"] = noiseParams.spikeProb;
      doc["spike_min"] = noiseParams.spikeMin;
      doc["spike_max"] = noiseParams.spikeMax;
    }
    JsonArray harmonics = doc.createNestedArray("harmonics");
    for (int i = 0; i < customHarmonicCount; i++) {
      JsonObject h = harmonics.createNestedObject();
      h["a"] = customHarmonics[i].amplitude;
      h["f"] = customHarmonics[i].frequency;
    }
    char payload[512];
    serializeJson(doc, payload, sizeof(payload));
    mqttCommonPublish(MQTT_TOPIC_SIGNAL, payload);
  } else {
    char payload[256];
    snprintf(payload, sizeof(payload),
      "{\"signal\":\"%s\",\"freq\":%.1f,\"freq_mult\":%.2f,\"running\":%s",
      SIGNAL_NAMES[currentSignal],
      (currentSignal == COMPOSITE) ? FREQ_1 : FREQ,
      freqMultiplier,
      signalRunning ? "true" : "false"
    );

    if (noiseEnabled) {
      char noise_part[128];
      snprintf(noise_part, sizeof(noise_part),
        ",\"noise_enabled\":true,\"sigma\":%.3f,\"spike_prob\":%.3f,\"spike_min\":%.2f,\"spike_max\":%.2f",
        noiseParams.sigma, noiseParams.spikeProb, noiseParams.spikeMin, noiseParams.spikeMax
      );
      strncat(payload, noise_part, sizeof(payload) - strlen(payload) - 1);
    }

    snprintf(payload + strlen(payload), sizeof(payload) - strlen(payload),
      ",\"timestamp\":%lu}", millis()
    );

    mqttCommonPublish(MQTT_TOPIC_SIGNAL, payload);
  }
}

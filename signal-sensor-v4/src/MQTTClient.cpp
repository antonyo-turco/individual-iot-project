#include "MQTTClient.h"
#include "Secrets.h"
#include "OLEDDisplay.h"
#include "BoardConfig.h"
#include "Benchmark.h"
#include "FFTProcessor.h"
#include <ArduinoJson.h>

#define MQTT_TOPIC_CMD "iot/sensor/command"

enum class MqttCommand {
  SetSampleRate,
  SetWindowSamples,
  Reset,
  StartBenchmark,
  SamplingDemo,
  ReleaseSampleRate,
  Unknown
};

struct CommandEntry {
  const char* name;
  MqttCommand cmd;
};

static constexpr CommandEntry COMMANDS[] = {
  { "set_sample_rate",    MqttCommand::SetSampleRate    },
  { "set_window_samples", MqttCommand::SetWindowSamples },
  { "reset",              MqttCommand::Reset            },
  { "start_benchmark",    MqttCommand::StartBenchmark   },
  { "sampling_demo",      MqttCommand::SamplingDemo     },
  { "release_sample_rate", MqttCommand::ReleaseSampleRate },
};

static MqttCommand parseCommand(const char* name) {
  for (const auto& entry : COMMANDS) {
    if (strcmp(name, entry.name) == 0) return entry.cmd;
  }
  return MqttCommand::Unknown;
}

static float* pCurrentSampleRate = nullptr;
static float* pLastDominantFreq = nullptr;
static bool sampleRateOverridden = false;

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

  const char* cmdStr = doc["cmd"];
  if (cmdStr == nullptr) {
    Serial.println("[MQTT] No 'cmd' field.");
    return;
  }

  switch (parseCommand(cmdStr)) {
    case MqttCommand::SetSampleRate:
      if (pCurrentSampleRate != nullptr) {
        *pCurrentSampleRate = constrain((float)doc["value"], 10.0f, SAMPLE_RATE_HZ);
        sampleRateOverridden = true;
        Serial.printf("[MQTT] Sample rate locked to %.1f Hz\n", *pCurrentSampleRate);
      }
      break;
    case MqttCommand::ReleaseSampleRate:
      sampleRateOverridden = false;
      Serial.println("[MQTT] Sample rate released to adaptive control.");
      break;
    case MqttCommand::SetWindowSamples:
      setWindowSamples((int)doc["value"]);
      break;
    case MqttCommand::Reset:
      Serial.println("[MQTT] Restarting...");
      ESP.restart();
      break;
    case MqttCommand::StartBenchmark:
      Serial.println("[MQTT] Starting max sampling benchmark...");
      runMaxSamplingBenchmark();
      break;
    case MqttCommand::SamplingDemo:
      if (pCurrentSampleRate != nullptr && pLastDominantFreq != nullptr) {
        Serial.println("[MQTT] Running sampling analysis demo...");
        runSamplingDemo(*pLastDominantFreq, *pCurrentSampleRate);
      }
      break;
    case MqttCommand::Unknown:
      Serial.printf("[MQTT] Unknown command: %s\n", cmdStr);
      break;
  }
}

static void statusDisplay(const char* msg) {
  showConnectionStatus(msg);
}

void initMQTT() {
  mqttCommonInit(WIFI_SSID, WIFI_PASSWORD, MQTT_BROKER, MQTT_PORT,
                 "heltec-v3", CA_CERT, statusDisplay);
  mqttCommonSetCallback(onMessage);
  mqttCommonSubscribe(MQTT_TOPIC_CMD);
}

void publishAggregate(float avg, float dominantHz, float sampleRateHz, uint32_t windowMs) {
  char payload[128];
  snprintf(payload, sizeof(payload),
    "{\"avg\":%.3f,\"dominant_hz\":%.2f,\"sample_rate_hz\":%.2f,\"window_ms\":%lu,\"timestamp\":%lu}",
    avg, dominantHz, sampleRateHz, windowMs, millis()
  );
  mqttCommonPublish(MQTT_TOPIC, payload);
}

void setSampleRatePtr(float* ptr) {
  pCurrentSampleRate = ptr;
}

void setDominantFreqPtr(float* ptr) {
  pLastDominantFreq = ptr;
}

bool isSampleRateOverridden() {
  return sampleRateOverridden;
}

void processMQTT() {
  mqttCommonLoop();
}

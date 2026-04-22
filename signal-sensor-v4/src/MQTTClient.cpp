#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "MQTTClient.h"
#include "Secrets.h"
#include "OLEDDisplay.h"
#include "BoardConfig.h"
#include "Benchmark.h"
#include "FFTProcessor.h"

#define MQTT_TOPIC_CMD "iot/sensor/command"
#define MQTT_CLIENT_ID "heltec-v4"

WiFiClientSecure net;
PubSubClient mqttClient(net);

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
  { "set_sample_rate",     MqttCommand::SetSampleRate     },
  { "set_window_samples",  MqttCommand::SetWindowSamples  },
  { "reset",               MqttCommand::Reset             },
  { "start_benchmark",     MqttCommand::StartBenchmark    },
  { "sampling_demo",       MqttCommand::SamplingDemo      },
  { "release_sample_rate", MqttCommand::ReleaseSampleRate },
};

static float* pCurrentSampleRate = nullptr;
static float* pLastDominantFreq  = nullptr;
static bool   sampleRateOverridden = false;

static MqttCommand parseCommand(const char* name) {
  for (const auto& entry : COMMANDS) {
    if (strcmp(name, entry.name) == 0) return entry.cmd;
  }
  return MqttCommand::Unknown;
}

static void callback(char* topic, byte* payload, unsigned int length) {
  char msg[length + 1];
  memcpy(msg, payload, length);
  msg[length] = '\0';

  Serial.print("[MQTT] Command received: ");
  Serial.println(msg);
  showBriefMessage(msg);

  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, msg);
  if (error) {
    Serial.print("[MQTT] JSON parse failed: ");
    Serial.println(error.f_str());
    return;
  }

  const char* cmdStr = doc["cmd"];
  if (!cmdStr) return;

  switch (parseCommand(cmdStr)) {
    case MqttCommand::SetSampleRate:
      if (pCurrentSampleRate != nullptr) {
        *pCurrentSampleRate = constrain((float)doc["value"], 10.0f, (float)SAMPLE_RATE_HZ);
        sampleRateOverridden = true;
        Serial.printf("[MQTT] Sample rate locked to %.1f Hz\n", *pCurrentSampleRate);
      }
      break;
    case MqttCommand::ReleaseSampleRate:
      sampleRateOverridden = false;
      Serial.println("[MQTT] Sample rate released.");
      break;
    case MqttCommand::SetWindowSamples:
      setWindowSamples((int)doc["value"]);
      break;
    case MqttCommand::Reset:
      Serial.println("[MQTT] Restarting...");
      ESP.restart();
      break;
    case MqttCommand::StartBenchmark:
      runMaxSamplingBenchmark();
      break;
    case MqttCommand::SamplingDemo:
      if (pCurrentSampleRate != nullptr && pLastDominantFreq != nullptr) {
        runSamplingDemo(*pLastDominantFreq, *pCurrentSampleRate);
      }
      break;
    default:
      Serial.printf("[MQTT] Unknown: %s\n", cmdStr);
      break;
  }
}

static void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("[MQTT] Attempting connection...");
    showConnectionStatus("Connecting MQTT...");
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("connected");
      showConnectionStatus("MQTT Online");
      mqttClient.subscribe(MQTT_TOPIC_CMD);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void initMQTT() {
  if (WiFi.status() != WL_CONNECTED) {
    showConnectionStatus("Connecting WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) { delay(500); }
    Serial.printf("\n[WiFi] Connected, IP: %s\n", WiFi.localIP().toString().c_str());
  }
  net.setCACert(CA_CERT);
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(callback);
  mqttClient.setBufferSize(256);
}

void processMQTT() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
}

void publishAggregate(float avg, float dominantHz, float sampleRateHz, uint32_t windowMs) {
  if (!mqttClient.connected()) return;
  char payload[128];
  snprintf(payload, sizeof(payload),
    "{\"avg\":%.3f,\"dominant_hz\":%.2f,\"sample_rate_hz\":%.2f,\"window_ms\":%lu,\"timestamp\":%lu}",
    avg, dominantHz, sampleRateHz, windowMs, millis()
  );
  mqttClient.publish(MQTT_TOPIC, payload);
}

void setSampleRatePtr(float* ptr) { pCurrentSampleRate = ptr; }
void setDominantFreqPtr(float* ptr) { pLastDominantFreq = ptr; }
bool isSampleRateOverridden() { return sampleRateOverridden; }

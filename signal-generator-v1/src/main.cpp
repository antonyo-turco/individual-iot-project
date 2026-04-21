#include <Arduino.h>
#include "Waveform.h"
#include "Secrets.h"
#include <MQTTCommon.h>
#include <ArduinoJson.h>

#define MQTT_TOPIC_SIGNAL  "iot/generator/signal"
#define MQTT_TOPIC_CMD     "iot/generator/command"

volatile bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
SignalType currentSignal = COMPOSITE;
volatile bool signalRunning = true;

void IRAM_ATTR onButtonPress() {
  buttonPressed = true;
}

static void publishSignalChange(SignalType signal) {
  char payload[128];
  snprintf(payload, sizeof(payload),
    "{\"signal\":\"%s\",\"freq\":%.1f,\"running\":%s,\"timestamp\":%lu}",
    SIGNAL_NAMES[signal],
    (signal == COMPOSITE) ? FREQ_1 : FREQ,
    signalRunning ? "true" : "false",
    millis()
  );
  mqttCommonPublish(MQTT_TOPIC_SIGNAL, payload);
}

static void onMessage(char* topic, byte* payload, unsigned int length) {
  char msg[length + 1];
  memcpy(msg, payload, length);
  msg[length] = '\0';

  Serial.print("[Generator] Command received: ");
  Serial.println(msg);

  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, msg) != DeserializationError::Ok) {
    Serial.println("[Generator] Failed to parse JSON.");
    return;
  }

  const char* cmd = doc["cmd"];
  if (cmd == nullptr) {
    Serial.println("[Generator] No 'cmd' field.");
    return;
  }

  if (strcmp(cmd, "start") == 0) {
    signalRunning = true;
    Serial.println("[Generator] Signal started.");
    publishSignalChange(currentSignal);
  } else if (strcmp(cmd, "stop") == 0) {
    signalRunning = false;
    dacWrite(DAC_PIN, 0);
    Serial.println("[Generator] Signal stopped.");
    publishSignalChange(currentSignal);
  } else if (strcmp(cmd, "set_signal") == 0) {
    const char* name = doc["value"];
    if (name != nullptr) {
      for (int i = 0; i < SIGNAL_COUNT; i++) {
        if (strcasecmp(name, SIGNAL_NAMES[i]) == 0) {
          currentSignal = (SignalType)i;
          Serial.printf("[Generator] Signal set to %s\n", SIGNAL_NAMES[i]);
          publishSignalChange(currentSignal);
          break;
        }
      }
    }
  } else {
    Serial.printf("[Generator] Unknown command: %s\n", cmd);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonPress, FALLING);
  delay(1000);

  mqttCommonInit(WIFI_SSID, WIFI_PASSWORD, MQTT_BROKER, MQTT_PORT, "esp32-generator", CA_CERT);
  mqttCommonSetCallback(onMessage);
  mqttCommonSubscribe(MQTT_TOPIC_CMD);

  Serial.print("Signal: ");
  Serial.println(SIGNAL_NAMES[currentSignal]);
  publishSignalChange(currentSignal);
}

void loop() {
  mqttCommonLoop();

  if (buttonPressed) {
    unsigned long now = millis();
    if (now - lastDebounceTime > DEBOUNCE_MS) {
      lastDebounceTime = now;
      currentSignal = nextSignal(currentSignal);
      Serial.print("Signal: ");
      Serial.println(SIGNAL_NAMES[currentSignal]);
      publishSignalChange(currentSignal);
    }
    buttonPressed = false;
  }

  if (signalRunning) {
    float t = millis() / 1000.0;
    float signal = constrain(computeSignal(currentSignal, t), 0, 255);
    dacWrite(DAC_PIN, (uint8_t)signal);

    int adcRaw = analogRead(ADC_PIN);
    Serial.print("DAC:");
    Serial.print((uint8_t)signal);
    Serial.print("\t");
    Serial.print("ADC:");
    Serial.println(adcRaw);
  }

  delay(SAMPLE_INTERVAL);
}
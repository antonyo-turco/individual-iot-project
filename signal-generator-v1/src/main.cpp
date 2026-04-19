#include <Arduino.h>
#include "Waveform.h"
#include "Secrets.h"
#include <MQTTCommon.h>
#include <ArduinoJson.h>

#define MQTT_TOPIC_SIGNAL "iot/generator/signal"

volatile bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
SignalType currentSignal = COMPOSITE;

void IRAM_ATTR onButtonPress() {
  buttonPressed = true;
}

static void publishSignalChange(SignalType signal) {
  char payload[128];
  snprintf(payload, sizeof(payload),
    "{\"signal\":\"%s\",\"freq\":%.1f,\"timestamp\":%lu}",
    SIGNAL_NAMES[signal],
    (signal == COMPOSITE) ? FREQ_1 : FREQ,
    millis()
  );
  mqttCommonPublish(MQTT_TOPIC_SIGNAL, payload);
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonPress, FALLING);
  delay(1000);

  mqttCommonInit(WIFI_SSID, WIFI_PASSWORD, MQTT_BROKER, MQTT_PORT, "esp32-generator", CA_CERT);

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

  float t = millis() / 1000.0;
  float signal = constrain(computeSignal(currentSignal, t), 0, 255);

  dacWrite(DAC_PIN, (uint8_t)signal);

  int adcRaw = analogRead(ADC_PIN);

  Serial.print("DAC:");
  Serial.print((uint8_t)signal);
  Serial.print("\t");
  Serial.print("ADC:");
  Serial.println(adcRaw);

  delay(SAMPLE_INTERVAL);
}
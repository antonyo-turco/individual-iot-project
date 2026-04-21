#include <Arduino.h>
#include "Waveform.h"
#include "MqttHandler.h"
#include "SignalManager.h"
#include "ButtonHandler.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  ButtonHandlerInit();
  SignalManagerInit();
  MqttInit();

  Serial.printf("[Main] Starting signal: %s\n", SIGNAL_NAMES[SignalManagerGetCurrentSignal()]);
  SignalManagerPublishState();
}

void loop() {
  MqttLoop();
  ButtonHandlerUpdate();

  if (SignalManagerIsRunning()) {
    float t = millis() / 1000.0;
    float signal = SignalManagerComputeSignal(t);
    signal = constrain(signal, 0, 255);
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
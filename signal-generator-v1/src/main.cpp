#include <Arduino.h>
#include "signal.h"

volatile bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
SignalType currentSignal = COMPOSITE;

void IRAM_ATTR onButtonPress() {
  buttonPressed = true;
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonPress, FALLING);
  delay(1000);
  Serial.print("Signal: ");
  Serial.println(SIGNAL_NAMES[currentSignal]);
}

void loop() {
  if (buttonPressed) {
    unsigned long now = millis();
    if (now - lastDebounceTime > DEBOUNCE_MS) {
      lastDebounceTime = now;
      currentSignal = nextSignal(currentSignal);
      Serial.print("Signal: ");
      Serial.println(SIGNAL_NAMES[currentSignal]);
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
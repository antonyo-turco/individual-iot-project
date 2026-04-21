#include <Arduino.h>
#include "ButtonHandler.h"
#include "SignalManager.h"
#include "Waveform.h"

static volatile bool buttonPressed = false;
static unsigned long lastDebounceTime = 0;

static void IRAM_ATTR onButtonPress() {
  buttonPressed = true;
}

void ButtonHandlerInit() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonPress, FALLING);
  Serial.println("[ButtonHandler] Initialized");
}

void ButtonHandlerUpdate() {
  if (buttonPressed) {
    unsigned long now = millis();
    if (now - lastDebounceTime > DEBOUNCE_MS) {
      lastDebounceTime = now;
      SignalManagerNextSignal();
    }
    buttonPressed = false;
  }
}

#include <Arduino.h>

const int LED_PIN = 2;
const int BLINK_INTERVAL = 500; // milliseconds

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n================================");
  Serial.println("Hello World!");
  Serial.println("LED Blink Test Starting...");
  Serial.println("================================\n");
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  // Turn LED on
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED ON");
  delay(BLINK_INTERVAL);
  
  // Turn LED off
  digitalWrite(LED_PIN, LOW);
  Serial.println("LED OFF");
  delay(BLINK_INTERVAL);
}
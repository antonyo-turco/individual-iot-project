#ifndef LORA_HANDLER_H
#define LORA_HANDLER_H

#include <Arduino.h>

// Initialise the SX1262 radio, perform OTAA join to TTN.
// Credentials are read from Secrets.h (LORA_APPEUI, LORA_DEVEUI, LORA_APPKEY).
// Blocks until the join succeeds or fails; logs result to Serial.
void initLoRa();

// Send a LoRaWAN uplink if joined and the per-uplink interval has elapsed.
// Call this every loop iteration alongside publishAggregate().
// Payload: dominant frequency (0.1 Hz units), avg mV, sample rate Hz – each as
// a big-endian uint16_t (6 bytes total, port 1).
void processLoRa(float avgMv, float dominantHz, float sampleRateHz);

#endif // LORA_HANDLER_H

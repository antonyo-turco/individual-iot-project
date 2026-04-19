#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <MQTTCommon.h>

void initMQTT();
void publishAggregate(float avg, float dominantHz, float sampleRateHz, uint32_t windowMs);
void setSampleRatePtr(float* ptr);
void setDominantFreqPtr(float* ptr);
void processMQTT();
bool isSampleRateOverridden();

#endif // MQTT_CLIENT_H

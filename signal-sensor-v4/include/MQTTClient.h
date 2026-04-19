#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>

void initMQTT();
void publishAggregate(float avg, float dominantHz, float sampleRateHz, uint32_t windowMs);

#endif // MQTT_CLIENT_H

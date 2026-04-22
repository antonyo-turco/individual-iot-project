#pragma once

#include "Waveform.h"

void blinkLed(int times);
void MqttInit();
void MqttLoop();
void MqttHandleMessage(char* topic, byte* payload, unsigned int length);

#pragma once

#include "Waveform.h"

struct NoiseParams {
  float sigma    = 0.2f;
  float spikeProb = 0.02f;
  float spikeMin  = 5.0f;
  float spikeMax  = 15.0f;
};

void SignalManagerInit();
void SignalManagerNextSignal();
void SignalManagerSetSignal(const char* name);
void SignalManagerStartSignal();
void SignalManagerStopSignal();
void SignalManagerPublishState();

void SignalManagerSetFreqMultiplier(float mult);
float SignalManagerGetFreqMultiplier();

void SignalManagerSetNoiseEnabled(bool enabled);
void SignalManagerSetNoiseParams(float sigma, float spikeProb, float spikeMin, float spikeMax);

void SignalManagerSetCustomSignal(CustomHarmonic* harmonics, int count);

SignalType SignalManagerGetCurrentSignal();
bool SignalManagerIsRunning();
bool SignalManagerIsNoiseEnabled();
NoiseParams SignalManagerGetNoiseParams();
float SignalManagerComputeSignal(float t);

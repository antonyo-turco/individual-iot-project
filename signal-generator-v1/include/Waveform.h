#pragma once

#ifdef __cplusplus

enum SignalType { COMPOSITE, SINE, SQUARE, TRIANGLE, SAWTOOTH, CUSTOM, FLAT, SIGNAL_COUNT };
static constexpr const char* SIGNAL_NAMES[] = { "Composite", "Sine", "Square", "Triangle", "Sawtooth", "Custom", "Flat" };

static constexpr int DAC_PIN = 25;
static constexpr int ADC_PIN = 34;
static constexpr int BUTTON_PIN = 0;

static constexpr float FREQ = 5.0;
static constexpr float FREQ_1 = 3.0;
static constexpr float FREQ_2 = 5.0;
static constexpr float AMPLITUDE = 100.0;
static constexpr float AMP_1 = 40.0;
static constexpr float AMP_2 = 80.0;
static constexpr float OFFSET = 127.0;
static constexpr float SCALE_FACTOR = 20.0f;

static constexpr int SAMPLE_RATE_HZ = 200;
static constexpr int SAMPLE_INTERVAL = 1000 / SAMPLE_RATE_HZ;
static constexpr unsigned long DEBOUNCE_MS = 50;

static constexpr int MAX_HARMONICS = 8;

struct CustomHarmonic {
  float amplitude;
  float frequency;
};

extern CustomHarmonic customHarmonics[MAX_HARMONICS];
extern int customHarmonicCount;

SignalType nextSignal(SignalType current);
float computeSignal(SignalType type, float t);

#endif

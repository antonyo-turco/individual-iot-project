#pragma once

#ifdef __cplusplus

enum SignalType { COMPOSITE, SINE, SQUARE, TRIANGLE, SAWTOOTH, SIGNAL_COUNT };
static constexpr const char* SIGNAL_NAMES[] = { "Composite", "Sine", "Square", "Triangle", "Sawtooth" };

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

static constexpr int SAMPLE_RATE_HZ = 200;
static constexpr int SAMPLE_INTERVAL = 1000 / SAMPLE_RATE_HZ;
static constexpr unsigned long DEBOUNCE_MS = 50;

SignalType nextSignal(SignalType current);
float computeSignal(SignalType type, float t);

#endif

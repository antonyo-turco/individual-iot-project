#include <Arduino.h>
#include "Waveform.h"
#include <cmath>

CustomHarmonic customHarmonics[MAX_HARMONICS];
int customHarmonicCount = 0;

SignalType nextSignal(SignalType current) {
  return static_cast<SignalType>((current + 1) % SIGNAL_COUNT);
}

float computeSignal(SignalType type, float t) {
  float phase = t * FREQ;
  phase -= floor(phase);

  switch (type) {
    case COMPOSITE:
      return OFFSET
           + AMP_1 * sin(2.0 * PI * FREQ_1 * t)
           + AMP_2 * sin(2.0 * PI * FREQ_2 * t);

    case SINE:
      return OFFSET + AMPLITUDE * sin(2.0 * PI * FREQ * t);

    case SQUARE:
      return OFFSET + (phase < 0.5f ? AMPLITUDE : -AMPLITUDE);

    case TRIANGLE:
      return OFFSET + AMPLITUDE * (phase < 0.5f
        ? (4.0f * phase - 1.0f)
        : (3.0f - 4.0f * phase));

    case SAWTOOTH:
      return OFFSET + AMPLITUDE * (2.0f * phase - 1.0f);

    case CUSTOM: {
      float s = 0.0f;
      for (int i = 0; i < customHarmonicCount; i++) {
        s += customHarmonics[i].amplitude * sin(2.0f * PI * customHarmonics[i].frequency * t);
      }
      return OFFSET + SCALE_FACTOR * s;
    }

    case FLAT:
      return OFFSET;

    default:
      return OFFSET;
  }
}

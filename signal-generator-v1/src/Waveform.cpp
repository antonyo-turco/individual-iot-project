#include <Arduino.h>
#include "Waveform.h"
#include <cmath>

CustomHarmonic customHarmonics[MAX_HARMONICS] = {
  {AMP_1 / SCALE_FACTOR, FREQ_1},
  {AMP_2 / SCALE_FACTOR, FREQ_2}
};
int customHarmonicCount = 2;

SignalType nextSignal(SignalType current) {
  return static_cast<SignalType>((current + 1) % SIGNAL_COUNT);
}

float computeSignal(SignalType type, float t, float freqMult) {
  float phase = t * FREQ * freqMult;
  phase -= floor(phase);

  switch (type) {
    case COMPOSITE:
      return OFFSET
           + AMP_1 * sin(2.0 * PI * FREQ_1 * freqMult * t)
           + AMP_2 * sin(2.0 * PI * FREQ_2 * freqMult * t);

    case SINE:
      return OFFSET + AMPLITUDE * sin(2.0 * PI * FREQ * freqMult * t);

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
        s += customHarmonics[i].amplitude * sin(2.0f * PI * customHarmonics[i].frequency * freqMult * t);
      }
      return OFFSET + SCALE_FACTOR * s;
    }

    case FLAT:
      return OFFSET;

    default:
      return OFFSET;
  }
}

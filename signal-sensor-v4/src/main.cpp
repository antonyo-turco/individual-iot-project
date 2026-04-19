#include <Arduino.h>
#include <BoardConfig.h>
#include <ADCReader.h>
#include <OLEDDisplay.h>
#include <FFTProcessor.h>
#include <MQTTClient.h>

enum DisplayState { WAVEFORM, FFT_INFO, FFT_SPECTRUM };
static DisplayState state = WAVEFORM;
static uint32_t stateStart = 0;
static float currentSampleRate = SAMPLE_RATE_HZ;
static float lastDominantFreq = 0.0f;
static float fftMagnitudes[64];
constexpr uint32_t FFT_SCREEN_MS = 5000;

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n=== S3 FFT Receiver ===");
  initADC();
  initOLED();
  initMQTT();
}

void loop() {
  bool fftReady = feedSample(currentSampleRate);

  switch (state) {
    case WAVEFORM: {
      float waveform[128];
      getRealtimeWaveform(waveform, 128);
      showWaveform(waveform, 128);

      if (fftReady) {
        lastDominantFreq = computeFFT();
        currentSampleRate = adaptSamplingRate(lastDominantFreq);
        getFFTMagnitudesForDisplay(fftMagnitudes, 64);
        publishAggregate(lastDominantFreq, lastDominantFreq, currentSampleRate, FFT_SCREEN_MS);
        state = FFT_INFO;
        stateStart = millis();
      }
      break;
    }
    case FFT_INFO:
      showFFTInfo(lastDominantFreq, currentSampleRate);
      if (millis() - stateStart >= FFT_SCREEN_MS) {
        state = FFT_SPECTRUM;
        stateStart = millis();
      }
      break;
    case FFT_SPECTRUM:
      showFFTSpectrum(lastDominantFreq, currentSampleRate, fftMagnitudes, 64);
      if (millis() - stateStart >= FFT_SCREEN_MS) {
        state = WAVEFORM;
      }
      break;
  }
}
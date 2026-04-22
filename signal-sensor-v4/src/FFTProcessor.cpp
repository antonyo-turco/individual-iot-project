#include <Arduino.h>
#include <arduinoFFT.h>
#include "FFTProcessor.h"
#include "ADCReader.h"
#include "BoardConfig.h"

static double vReal[FFT_SAMPLES];
static double vImag[FFT_SAMPLES];
// Sample rate passed dynamically; constructor value only used for windowing/compute
static ArduinoFFT<double> FFT(vReal, vImag, FFT_SAMPLES, SAMPLE_RATE_HZ);

static constexpr int CIRC_BUF_SIZE = FFT_SAMPLES;
static float circBuf[CIRC_BUF_SIZE];
static int circHead = 0;
static int fftIndex = 0;
static uint32_t lastSampleTimeUs = 0;
static int activeWindowSamples = FFT_SAMPLES;
static float lastAvgMv = 0.0f;

void setWindowSamples(int count) {
  activeWindowSamples = constrain(count, 64, FFT_SAMPLES);
  fftIndex = 0;
  Serial.printf("[FFT] Window samples set to %d\n", activeWindowSamples);
}

static float lastConfiguredRate = 0.0f;

bool feedSample(float sampleRateHz) {
  // Discard any partial buffer collected at the old rate — mixed-rate FFT is garbage
  if (sampleRateHz != lastConfiguredRate) {
    fftIndex = 0;
    lastConfiguredRate = sampleRateHz;
  }

  uint32_t intervalUs = static_cast<uint32_t>(1000000.0f / sampleRateHz);
  uint32_t now = micros();
  if (now - lastSampleTimeUs < intervalUs) return false;
  lastSampleTimeUs = now;

  uint32_t mV = readAdcMillivolts();
  float val = static_cast<float>(mV);

  circBuf[circHead] = val;
  circHead = (circHead + 1) % CIRC_BUF_SIZE;

  vReal[fftIndex] = static_cast<double>(mV);
  vImag[fftIndex] = 0.0;
  fftIndex++;

  if (fftIndex >= activeWindowSamples) {
    fftIndex = 0;
    double mean = 0.0;
    for (int i = 0; i < activeWindowSamples; i++) mean += vReal[i];
    mean /= activeWindowSamples;
    lastAvgMv = static_cast<float>(mean);
    for (int i = 0; i < activeWindowSamples; i++) vReal[i] -= mean;
    return true;
  }
  return false;
}

float getLastAvgMv() {
  return lastAvgMv;
}

float computeFFT(float sampleRateHz) {
  FFT.windowing(FFTWindow::Hann, FFTDirection::Forward);
  FFT.compute(FFTDirection::Forward);
  FFT.complexToMagnitude();

  // Use the ACTUAL current sample rate for the frequency axis
  float freqResolution = sampleRateHz / activeWindowSamples;

  Serial.println("\n--- FFT Spectrum ---");
  for (int i = 1; i < activeWindowSamples / 2; i++) {
    if (vReal[i] > 100.0) {
      float freq = i * freqResolution;
      Serial.print("  ");
      Serial.print(freq, 2);
      Serial.print(" Hz -> magnitude: ");
      Serial.println(vReal[i], 1);
    }
  }

  // Find peak bin manually using correct frequency axis
  int peakBin = 1;
  for (int i = 2; i < activeWindowSamples / 2; i++) {
    if (vReal[i] > vReal[peakBin]) peakBin = i;
  }
  float dominantFreq = peakBin * freqResolution;

  Serial.print("  Dominant frequency: ");
  Serial.print(dominantFreq, 2);
  Serial.println(" Hz");
  Serial.println("--------------------\n");

  return dominantFreq;
}

// Require 3 consecutive FFT windows in the same frequency band before adapting down.
// Adapting up (more bandwidth needed) is immediate.
float adaptSamplingRate(float dominantFreq, float currentRate) {
  static float history[3] = {0, 0, 0};
  static int   histIdx    = 0;

  history[histIdx] = dominantFreq;
  histIdx = (histIdx + 1) % 3;

  // 5x safety margin = 2.5x above Nyquist; much less likely to alias on noise spikes
  float targetRate = dominantFreq * 5.0f;
  targetRate = constrain(targetRate, 10.0f, SAMPLE_RATE_HZ);

  if (targetRate >= currentRate) {
    // Always adapt up immediately
    Serial.printf("[ADAPT] Rate up: %.1f Hz\n", targetRate);
    return targetRate;
  }

  // Only adapt down if all 3 recent windows agree on a similar target
  float minTarget = history[0] * 5.0f, maxTarget = history[0] * 5.0f;
  for (int i = 1; i < 3; i++) {
    float t = history[i] * 5.0f;
    if (t < minTarget) minTarget = t;
    if (t > maxTarget) maxTarget = t;
  }
  bool stable = (maxTarget <= minTarget * 1.5f) && (minTarget > 0);
  if (!stable) {
    Serial.printf("[ADAPT] Waiting for stable estimate (current %.1f Hz)\n", currentRate);
    return currentRate;
  }

  // Cap step-down to 2x per window — prevents 200->12.5 Hz in one jump
  float steppedRate = fmaxf(targetRate, currentRate / 2.0f);
  Serial.printf("[ADAPT] Rate down: %.1f Hz\n", steppedRate);
  return steppedRate;
}

void getFFTMagnitudesForDisplay(float* out, int count) {
  int usefulBins = FFT_SAMPLES / 2;
  float maxMag = 1.0f;
  for (int i = 1; i < usefulBins; i++) {
    if (vReal[i] > maxMag) maxMag = static_cast<float>(vReal[i]);
  }

  for (int i = 0; i < count; i++) {
    int binIndex = 1 + (i * (usefulBins - 1)) / count;
    out[i] = static_cast<float>(vReal[binIndex]) / maxMag;
  }
}

void getRealtimeWaveform(float* out, int count) {
  float minVal = circBuf[0];
  float maxVal = circBuf[0];
  for (int i = 1; i < CIRC_BUF_SIZE; i++) {
    if (circBuf[i] < minVal) minVal = circBuf[i];
    if (circBuf[i] > maxVal) maxVal = circBuf[i];
  }
  float range = maxVal - minVal;
  if (range < 1.0f) range = 1.0f;

  for (int i = 0; i < count; i++) {
    int idx = (circHead - count + i + CIRC_BUF_SIZE) % CIRC_BUF_SIZE;
    out[i] = (circBuf[idx] - minVal) / range;
  }
}

void getWaveformRange(float* minMv, float* maxMv) {
  float minVal = circBuf[0];
  float maxVal = circBuf[0];
  for (int i = 1; i < CIRC_BUF_SIZE; i++) {
    if (circBuf[i] < minVal) minVal = circBuf[i];
    if (circBuf[i] > maxVal) maxVal = circBuf[i];
  }
  *minMv = minVal;
  *maxMv = maxVal;
}

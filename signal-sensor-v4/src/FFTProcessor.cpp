#include <Arduino.h>
#include <arduinoFFT.h>
#include "FFTProcessor.h"
#include "ADCReader.h"
#include "BoardConfig.h"

static double vReal[FFT_SAMPLES];
static double vImag[FFT_SAMPLES];
static ArduinoFFT<double> FFT(vReal, vImag, FFT_SAMPLES, SAMPLE_RATE_HZ);

static constexpr int CIRC_BUF_SIZE = FFT_SAMPLES;
static float circBuf[CIRC_BUF_SIZE];
static int circHead = 0;
static int fftIndex = 0;
static uint32_t lastSampleTimeUs = 0;
static int activeWindowSamples = FFT_SAMPLES;

void setWindowSamples(int count) {
  activeWindowSamples = constrain(count, 64, FFT_SAMPLES);
  fftIndex = 0;
  Serial.printf("[FFT] Window samples set to %d\n", activeWindowSamples);
}

bool feedSample(float sampleRateHz) {
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
    for (int i = 0; i < activeWindowSamples; i++) vReal[i] -= mean;
    return true;
  }
  return false;
}

float computeFFT() {
  FFT.windowing(FFTWindow::Hann, FFTDirection::Forward);
  FFT.compute(FFTDirection::Forward);
  FFT.complexToMagnitude();

  Serial.println("\n--- FFT Spectrum ---");
  float freqResolution = SAMPLE_RATE_HZ / FFT_SAMPLES;
  for (int i = 1; i < FFT_SAMPLES / 2; i++) {
    if (vReal[i] > 100.0) {
      float freq = i * freqResolution;
      Serial.print("  ");
      Serial.print(freq, 2);
      Serial.print(" Hz -> magnitude: ");
      Serial.println(vReal[i], 1);
    }
  }

  float dominantFreq = FFT.majorPeak();
  Serial.print("  Dominant frequency: ");
  Serial.print(dominantFreq, 2);
  Serial.println(" Hz");
  Serial.println("--------------------\n");

  return dominantFreq;
}

float adaptSamplingRate(float dominantFreq) {
  float newRate = dominantFreq * 2.5f;
  newRate = constrain(newRate, 10.0f, SAMPLE_RATE_HZ);

  Serial.print("[ADAPT] New sampling rate: ");
  Serial.print(newRate, 1);
  Serial.println(" Hz");

  return newRate;
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

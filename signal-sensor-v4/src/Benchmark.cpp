#include "Benchmark.h"
#include "ADCReader.h"
#include "BoardConfig.h"
#include "MQTTClient.h"
#include "OLEDDisplay.h"

float measureMaxSamplingRate() {
  constexpr int BENCH_SAMPLES = 10000;
  uint32_t start = micros();
  for (int i = 0; i < BENCH_SAMPLES; i++) {
    readRawADC();
  }
  uint32_t elapsed = micros() - start;
  float achievedHz = (BENCH_SAMPLES / (float)elapsed) * 1000000.0f;
  Serial.printf("[BENCH] Max sampling rate: %.1f Hz\n", achievedHz);
  return achievedHz;
}

void runMaxSamplingBenchmark() {
  constexpr int BENCH_SAMPLES = 10000;
  uint32_t start = micros();

  for (int i = 0; i < BENCH_SAMPLES; i++) {
    readRawADC();
  }

  uint32_t elapsed = micros() - start;
  float achievedHz = (BENCH_SAMPLES / (float)elapsed) * 1000000.0f;

  Serial.printf("[BENCH] %d samples in %lu us = %.1f Hz\n",
    BENCH_SAMPLES, elapsed, achievedHz);

  showMaxSamplingResult(achievedHz, BENCH_SAMPLES, elapsed);
  delay(3000);

  char payload[256];
  snprintf(payload, sizeof(payload),
    "{\"type\":\"max_sampling\",\"hz\":%.1f,\"samples\":%d,\"elapsed_us\":%lu}",
    achievedHz, BENCH_SAMPLES, elapsed);

  if (mqttClient.connected()) {
    mqttClient.publish("iot/sensor/benchmark", payload);
    Serial.print("[MQTT] Published benchmark: ");
    Serial.println(payload);
  }
}

void runSamplingDemo(float dominantFreq, float currentSampleRate) {
  Serial.println("\n=== SAMPLING RATE ANALYSIS ===");

  float nyquistRate = dominantFreq * 2.0f;
  Serial.printf("Dominant freq: %.2f Hz\n", dominantFreq);
  Serial.printf("Nyquist rate (2x): %.2f Hz\n", nyquistRate);
  Serial.printf("Current adaptive rate: %.1f Hz\n", currentSampleRate);

  constexpr int WINDOW_SAMPLES = FFT_SAMPLES;
  float nyquistWindowMs = (WINDOW_SAMPLES / nyquistRate) * 1000.0f;
  float adaptiveWindowMs = (WINDOW_SAMPLES / currentSampleRate) * 1000.0f;

  Serial.printf("Nyquist window size: %.0f ms (%d samples)\n", nyquistWindowMs, WINDOW_SAMPLES);
  Serial.printf("Adaptive window size: %.0f ms (%d samples)\n", adaptiveWindowMs, WINDOW_SAMPLES);

  float reductionFactor = currentSampleRate / SAMPLE_RATE_HZ;
  Serial.printf("Data reduction: %.1f%% of naive (200 Hz)\n", reductionFactor * 100.0f);
  Serial.println("=============================\n");

  showSamplingAnalysis(dominantFreq, nyquistRate, currentSampleRate, reductionFactor * 100.0f);
  delay(3000);

  char payload[512];
  snprintf(payload, sizeof(payload),
    "{\"type\":\"sampling_analysis\",\"dominant_hz\":%.2f,\"nyquist_hz\":%.2f,\"adaptive_hz\":%.1f,\"nyquist_window_ms\":%.0f,\"adaptive_window_ms\":%.0f,\"reduction_percent\":%.1f}",
    dominantFreq, nyquistRate, currentSampleRate, nyquistWindowMs, adaptiveWindowMs, reductionFactor * 100.0f);

  if (mqttClient.connected()) {
    mqttClient.publish("iot/sensor/benchmark", payload);
    Serial.print("[MQTT] Published analysis: ");
    Serial.println(payload);
  }
}

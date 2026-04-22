#include <Arduino.h>
#include <BoardConfig.h>
#include <ADCReader.h>
#include <OLEDDisplay.h>
#include <FFTProcessor.h>
#include <MQTTClient.h>
#include <LoRaHandler.h>

enum DisplayState { WAVEFORM, FFT_INFO, FFT_SPECTRUM };
static DisplayState state = WAVEFORM;
static float currentSampleRate = SAMPLE_RATE_HZ;
static float lastDominantFreq = 0.0f;
static float fftMagnitudes[64];
constexpr uint32_t FFT_SCREEN_MS = 5000;
static uint32_t ledOffAt = 0;

static bool lastButtonState = HIGH;

static bool buttonPressed() {
  bool current = digitalRead(BUTTON_PIN);
  bool pressed = (lastButtonState == HIGH && current == LOW);
  lastButtonState = current;
  return pressed;
}

void taskFFT(void* pvParameters);
void taskMQTT(void* pvParameters);


void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n=== S3 FFT Receiver ===");
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  initADC();
  initOLED();
  initMQTT();
  initLoRa();
  setSampleRatePtr(&currentSampleRate);
  setDominantFreqPtr(&lastDominantFreq);
  xTaskCreatePinnedToCore(taskFFT,      "FFT",       10000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(taskMQTT,     "MQTT-WiFi", 10000, NULL, 1, NULL, 0);
}

void taskMQTT(void* pvParameters) {
  while (1) {
    processMQTT();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void taskFFT(void* pvParameters) {
  while (1) {
    bool fftReady = feedSample(currentSampleRate);
    bool btn = buttonPressed();

    switch (state) {
      case WAVEFORM: {
        float waveform[128];
        float wMin, wMax;
        getRealtimeWaveform(waveform, 128);
        getWaveformRange(&wMin, &wMax);
        showWaveform(waveform, 128, wMin, wMax);

        if (fftReady) {
          digitalWrite(LED_PIN, HIGH);
          ledOffAt = millis() + 500;
          lastDominantFreq = computeFFT();
          if (!isSampleRateOverridden()) {
            currentSampleRate = adaptSamplingRate(lastDominantFreq);
          }
          getFFTMagnitudesForDisplay(fftMagnitudes, 64);
          publishAggregate(getLastAvgMv(), lastDominantFreq, currentSampleRate, FFT_SCREEN_MS);
          processLoRa(getLastAvgMv(), lastDominantFreq, currentSampleRate);
        }
        if (btn) {
          state = FFT_INFO;
        }
        break;
      }
      case FFT_INFO:
        showFFTInfo(lastDominantFreq, currentSampleRate);
        if (btn) {
          state = FFT_SPECTRUM;
        }
        break;
      case FFT_SPECTRUM:
        showFFTSpectrum(lastDominantFreq, currentSampleRate, fftMagnitudes, 64);
        if (btn) {
          state = WAVEFORM;
        }
        break;
    }

    if (ledOffAt > 0 && millis() >= ledOffAt) {
      digitalWrite(LED_PIN, LOW);
      ledOffAt = 0;
    }
  }
}

void loop() {
  vTaskDelete(NULL);
}

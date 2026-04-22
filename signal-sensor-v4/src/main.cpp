#include <Arduino.h>
#include <BoardConfig.h>
#include <ADCReader.h>
#include <OLEDDisplay.h>
#include <FFTProcessor.h>
#include <MQTTClient.h>
#include <LoRaHandler.h>
#include <Benchmark.h>

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

// Inter-task FFT result buffer (taskFFT writes, taskIO reads)
static SemaphoreHandle_t gFFTMutex;
static volatile bool     gFFTReady   = false;
static float             gAvgMv      = 0.0f;
static float             gDomFreq    = 0.0f;
static float             gSampleRate = SAMPLE_RATE_HZ;
static float             gMagnitudes[64];

void taskFFT(void* pvParameters);
void taskIO(void* pvParameters);

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n=== S3 FFT Receiver ===");
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  gFFTMutex = xSemaphoreCreateMutex();
  initADC();
  initOLED();
  // Measure the hardware's max ADC throughput and use it as the starting rate
  float maxHz = measureMaxSamplingRate();
  currentSampleRate = maxHz;
  gSampleRate       = maxHz;
  initMQTT();
  initLoRa();
  setSampleRatePtr(&currentSampleRate);
  setDominantFreqPtr(&lastDominantFreq);
  xTaskCreatePinnedToCore(taskFFT, "FFT", 10000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(taskIO,  "IO",  10000, NULL, 1, NULL, 0);
}

// Core 1 — ADC sampling + FFT computation only
void taskFFT(void* pvParameters) {
  while (1) {
    bool fftReady = feedSample(currentSampleRate);
    if (fftReady) {
      float domFreq = computeFFT(currentSampleRate);
      float avgMv   = getLastAvgMv();
      if (!isSampleRateOverridden()) {
        currentSampleRate = adaptSamplingRate(domFreq, currentSampleRate);
      }
      if (xSemaphoreTake(gFFTMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        gDomFreq    = domFreq;
        gAvgMv      = avgMv;
        gSampleRate = currentSampleRate;
        getFFTMagnitudesForDisplay(gMagnitudes, 64);
        gFFTReady   = true;
        xSemaphoreGive(gFFTMutex);
      }
    }
  }
}

// Core 0 — MQTT, LoRa, display, button, LED
void taskIO(void* pvParameters) {
  while (1) {
    //  MQTT 
    processMQTT();

    // Consume new FFT results 
    if (xSemaphoreTake(gFFTMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
      bool newData  = gFFTReady;
      float domFreq = gDomFreq, avgMv = gAvgMv, sr = gSampleRate;
      float mags[64];
      if (newData) {
        memcpy(mags, gMagnitudes, sizeof(mags));
        gFFTReady = false;
      }
      xSemaphoreGive(gFFTMutex);

      if (newData) {
        lastDominantFreq  = domFreq;
        currentSampleRate = sr;
        memcpy(fftMagnitudes, mags, sizeof(fftMagnitudes));
        digitalWrite(LED_PIN, HIGH);
        ledOffAt = millis() + 500;
        publishAggregate(avgMv, domFreq, sr, FFT_SCREEN_MS);
        processLoRa(avgMv, domFreq, sr);
      }
    }

    // Button 
    if (buttonPressed()) {
      state = static_cast<DisplayState>((state + 1) % 3);
    }

    // Display 
    switch (state) {
      case WAVEFORM: {
        float waveform[128];
        float wMin, wMax;
        getRealtimeWaveform(waveform, 128);
        getWaveformRange(&wMin, &wMax);
        showWaveform(waveform, 128, wMin, wMax);
        break;
      }
      case FFT_INFO:
        showFFTInfo(lastDominantFreq, currentSampleRate);
        break;
      case FFT_SPECTRUM:
        showFFTSpectrum(lastDominantFreq, currentSampleRate, fftMagnitudes, 64);
        break;
    }

    // LED off timer 
    if (ledOffAt > 0 && millis() >= ledOffAt) {
      digitalWrite(LED_PIN, LOW);
      ledOffAt = 0;
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void loop() {
  vTaskDelete(NULL);
}

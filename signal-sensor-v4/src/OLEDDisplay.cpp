#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "OLEDDisplay.h"
#include "BoardConfig.h"

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
static bool displayReady = false;

void initOLED() {
  pinMode(VEXT_PIN, OUTPUT);
  digitalWrite(VEXT_PIN, LOW);
  delay(100);

  Wire.begin(OLED_SDA, OLED_SCL);
  delay(100);

  displayReady = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (!displayReady) {
    Serial.println("[OLED] ERROR: begin() failed.");
    return;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("FFT ready...");
  display.display();
  Serial.println("[OLED] OK.");
}

void showWaveform(const float* waveform, int count) {
  if (!displayReady) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Waveform");

  constexpr int graphTop = 10;
  constexpr int graphH = SCREEN_HEIGHT - graphTop;
  for (int i = 1; i < count; i++) {
    int x0 = (i - 1) * SCREEN_WIDTH / count;
    int x1 = i       * SCREEN_WIDTH / count;
    int y0 = graphTop + graphH - static_cast<int>(waveform[i - 1] * graphH);
    int y1 = graphTop + graphH - static_cast<int>(waveform[i]     * graphH);
    display.drawLine(x0, y0, x1, y1, SSD1306_WHITE);
  }
  display.display();
}

void showFFTInfo(float dominantFreq, float samplingFreq) {
  if (!displayReady) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("Dominant: ");
  display.print(dominantFreq, 1);
  display.println(" Hz");
  display.setCursor(0, 12);
  display.print("Sampling: ");
  display.print(samplingFreq, 0);
  display.println(" Hz");
  display.setCursor(0, 24);
  display.print("Nyquist:  ");
  display.print(samplingFreq / 2.0f, 0);
  display.println(" Hz");
  display.setCursor(0, 36);
  display.print("Res:     ");
  display.print(samplingFreq / FFT_SAMPLES, 2);
  display.println(" Hz/bin");
  display.display();
}

void showFFTSpectrum(float dominantFreq, float samplingFreq, const float* fftMagnitudes, int fftCount) {
  if (!displayReady) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("Spectrum  pk:");
  display.print(dominantFreq, 0);
  display.print("Hz");

  constexpr int graphTop = 10;
  constexpr int graphBottom = 55;
  constexpr int graphH = graphBottom - graphTop;
  int barWidth = SCREEN_WIDTH / fftCount;
  if (barWidth < 1) barWidth = 1;

  for (int i = 0; i < fftCount; i++) {
    int barH = static_cast<int>(fftMagnitudes[i] * graphH);
    if (barH > 0) {
      display.fillRect(i * barWidth, graphBottom - barH, barWidth, barH, SSD1306_WHITE);
    }
  }

  float nyquist = samplingFreq / 2.0f;
  constexpr int labelY = 57;
  display.setCursor(0, labelY);
  display.print("0");
  int midX = SCREEN_WIDTH / 2 - 12;
  display.setCursor(midX, labelY);
  display.print(static_cast<int>(nyquist / 2));
  int endX = SCREEN_WIDTH - 24;
  display.setCursor(endX, labelY);
  display.print(static_cast<int>(nyquist));

  display.display();
}

void showConnectionStatus(const char* status) {
  if (!displayReady) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connecting...");
  display.setCursor(0, 20);
  display.println(status);
  display.display();
}

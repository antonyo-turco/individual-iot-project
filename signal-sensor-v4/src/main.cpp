#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

//  Pin Definitions for Heltec WiFi LoRa 32 V3 
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_SDA      17   // I2C data
#define OLED_SCL      18   // I2C clock
#define OLED_RESET    21   // Display reset
#define OLED_ADDR   0x3C   // Default I2C address

// VExt (GPIO 36) controls power to the OLED and other on-board peripherals.
// Setting it LOW enables 3.3 V supply — MUST be done before display.begin().
#define VEXT_PIN      36

// Built-in white LED on the Heltec V3
#define LED_PIN       35

//  Display Object 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Helpers 
void testLED() {
  Serial.println("[LED] Blinking built-in LED 5 times...");
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }
  Serial.println("[LED] Done.");
}

void testOLED() {
  Serial.println("[OLED] Initialising display...");

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[OLED] ERROR: display.begin() failed. Check wiring and I2C address.");
    return; // Don't halt — keep LED and Serial tests alive
  }

  Serial.println("[OLED] Display OK.");
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println("Hello");
  display.setCursor(10, 38);
  display.println("World!");
  display.display();
  Serial.println("[OLED] 'Hello World' written to screen.");
}

//  Setup
void setup() {
  // 1. Serial — USB CDC on ESP32-S3 needs a short delay after boot
  Serial.begin(115200);
  delay(1500);  // Wait for USB CDC to enumerate on host
  Serial.println("\n\n=== Heltec V3 Board Test ===");

  // 2. Enable VExt to power the OLED (and external peripherals)
  //    Pull GPIO 36 LOW to switch on 3.3 V rail
  pinMode(VEXT_PIN, OUTPUT);
  digitalWrite(VEXT_PIN, LOW);  // LOW = VExt ON
  delay(100);                   // Short settle time
  Serial.println("[PWR] VExt enabled (GPIO 36 LOW).");

  // 3. LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 4. I2C
  Wire.begin(OLED_SDA, OLED_SCL);
  delay(100);
  Serial.println("[I2C] Bus started on SDA=17, SCL=18.");

  // 5. Run tests
  testLED();
  testOLED();

  Serial.println("\n=== Setup complete. Entering loop. ===");
}

// Loop 
void loop() {
  // Heartbeat: print uptime every 5 seconds so you can confirm Serial is alive
  Serial.print("[ALIVE] Uptime: ");
  Serial.print(millis() / 1000);
  Serial.println(" s");
  delay(5000);
}
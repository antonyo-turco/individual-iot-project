#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <Arduino.h>
#include <driver/adc.h>

constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
constexpr int OLED_SDA = 17;
constexpr int OLED_SCL = 18;
constexpr int OLED_RESET = 21;
constexpr uint8_t OLED_ADDR = 0x3C;
constexpr int VEXT_PIN = 36;
constexpr int LED_PIN = 35;  // Active-high: HIGH = on, LOW = off
constexpr int BUTTON_PIN = 0;

constexpr adc1_channel_t ADC_CHANNEL = ADC1_CHANNEL_1;
constexpr adc_atten_t ADC_ATTEN = ADC_ATTEN_DB_12;
constexpr adc_bits_width_t ADC_WIDTH = ADC_WIDTH_BIT_12;
constexpr adc_unit_t ADC_UNIT = ADC_UNIT_1;
constexpr int ADC_REF_MV = 1100;

constexpr int FFT_SAMPLES = 256;
constexpr float SAMPLE_RATE_HZ = 200.0f;
constexpr int SAMPLE_INTERVAL_US = 5000;

#endif // BOARD_CONFIG_H

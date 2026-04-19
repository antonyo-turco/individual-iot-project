#include "ADCReader.h"
#include "BoardConfig.h"
#include <esp_adc_cal.h>

static esp_adc_cal_characteristics_t adcChars;

void initADC() {
  adc1_config_width(ADC_WIDTH);
  adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);
  esp_adc_cal_characterize(ADC_UNIT, ADC_ATTEN, ADC_WIDTH, ADC_REF_MV, &adcChars);
  Serial.println("[ADC] Calibrated OK.");
}

int readRawADC() {
  return adc1_get_raw(ADC_CHANNEL);
}

uint32_t readAdcMillivolts() {
  return esp_adc_cal_raw_to_voltage(readRawADC(), &adcChars);
}

#ifndef ADC_READER_H
#define ADC_READER_H

#include <Arduino.h>

void initADC();
int readRawADC();
uint32_t readAdcMillivolts();

#endif // ADC_READER_H

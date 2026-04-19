#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

void initOLED();
void showWaveform(const float* waveform, int count);
void showFFTInfo(float dominantFreq, float samplingFreq);
void showFFTSpectrum(float dominantFreq, float samplingFreq, const float* fftMagnitudes, int fftCount);
void showConnectionStatus(const char* status);

#endif // OLED_DISPLAY_H

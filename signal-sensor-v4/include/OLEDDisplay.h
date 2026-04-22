#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

void initOLED();
void showWaveform(const float* waveform, int count, float minMv, float maxMv);
void showFFTInfo(float dominantFreq, float samplingFreq);
void showFFTSpectrum(float dominantFreq, float samplingFreq, const float* fftMagnitudes, int fftCount);
void showConnectionStatus(const char* status);
void showBriefMessage(const char* msg);
void showMaxSamplingResult(float hz, int samples, uint32_t elapsedUs);
void showSamplingAnalysis(float dominantHz, float nyquistHz, float adaptiveHz, float reductionPercent);

#endif // OLED_DISPLAY_H

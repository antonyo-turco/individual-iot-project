#ifndef FFT_PROCESSOR_H
#define FFT_PROCESSOR_H

bool feedSample(float sampleRateHz);
float computeFFT(float sampleRateHz);
float adaptSamplingRate(float dominantFreq, float currentRate);
void getFFTMagnitudesForDisplay(float* out, int count);
void getRealtimeWaveform(float* out, int count);
void getWaveformRange(float* minMv, float* maxMv);
void setWindowSamples(int count);
int  getWindowSamples();
float getLastAvgMv();

#endif // FFT_PROCESSOR_H

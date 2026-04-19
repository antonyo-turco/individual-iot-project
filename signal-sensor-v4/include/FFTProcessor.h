#ifndef FFT_PROCESSOR_H
#define FFT_PROCESSOR_H

bool feedSample(float sampleRateHz);
float computeFFT();
float adaptSamplingRate(float dominantFreq);
void getFFTMagnitudesForDisplay(float* out, int count);
void getRealtimeWaveform(float* out, int count);
void setWindowSamples(int count);

#endif // FFT_PROCESSOR_H

#ifndef AUDIOPLOT_DR_WAV_H
#define AUDIOPLOT_DR_WAV_H

#include <cstdint>

float* openWavFileAndReadPcmFramesF32(const char* filename, unsigned int* channels, unsigned int* sampleRate, uint64_t* totalFrameCount);
void freeWavSampleData(float* pSampleData);

#endif // AUDIOPLOT_DR_WAV_H
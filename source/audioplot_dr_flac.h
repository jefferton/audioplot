#ifndef AUDIOPLOT_DR_FLAC_H
#define AUDIOPLOT_DR_FLAC_H

#include <cstdint>

float* openFlacFileAndReadPcmFramesF32(const char* filename, unsigned int* channels, unsigned int* sampleRate, uint64_t* totalFrameCount);
void freeFlacSampleData(float* pSampleData);

#endif // AUDIOPLOT_DR_FLAC_H
#ifndef AUDIOPLOT_DR_MP3_H
#define AUDIOPLOT_DR_MP3_H

#include <cstdint>

float* openMp3FileAndReadPcmFramesF32(const char* filename, unsigned int* channels, unsigned int* sampleRate, uint64_t* totalFrameCount);
void freeMp3SampleData(float* pSampleData);

#endif // AUDIOPLOT_DR_MP3_H
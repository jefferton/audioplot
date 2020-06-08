#ifndef AUDIOPLOT_STB_VORBIS_H
#define AUDIOPLOT_STB_VORBIS_H

#include <cinttypes>

float* openOggFileAndReadPcmFramesF32(const char* filename, unsigned int* channels,
                                      unsigned int* sampleRate, uint64_t* totalFrameCount);

void freeOggSampleData(float* pSampleData);

#endif // AUDIOPLOT_STB_VORBIS_H
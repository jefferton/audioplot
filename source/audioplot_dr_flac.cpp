#include "audioplot_dr_flac.h"

#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"

float* openFlacFileAndReadPcmFramesF32(const char* filename, unsigned int* channels, unsigned int* sampleRate, uint64_t* totalFrameCount)
{
    return drflac_open_file_and_read_pcm_frames_f32(filename, channels, sampleRate, totalFrameCount, NULL);
}

void freeFlacSampleData(float* pSampleData)
{
    drflac_free(pSampleData, NULL);
}
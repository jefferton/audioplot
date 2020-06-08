#include "audioplot_dr_wav.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

float* openWavFileAndReadPcmFramesF32(const char* filename, unsigned int* channels, unsigned int* sampleRate, uint64_t* totalFrameCount)
{
    return drwav_open_file_and_read_pcm_frames_f32(filename, channels, sampleRate, totalFrameCount, NULL);
}

void freeWavSampleData(float* pSampleData)
{
    drwav_free(pSampleData, NULL);
}
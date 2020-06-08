#include "audioplot_dr_mp3.h"

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

float* openMp3FileAndReadPcmFramesF32(const char* filename, unsigned int* channels, unsigned int* sampleRate, uint64_t* totalFrameCount)
{
    drmp3_config config;
    float* pSamples = drmp3_open_file_and_read_pcm_frames_f32(filename, &config, totalFrameCount, NULL);
    *channels = config.channels;
    *sampleRate = config.sampleRate;
    return pSamples;
}

void freeMp3SampleData(float* pSampleData)
{
    drmp3_free(pSampleData, NULL);
}
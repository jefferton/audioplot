#include "audioplot_stb_vorbis.h"

#include "stb_vorbis.c"

#include <cinttypes>
#include <vector>

#define stb_clamp(x,xmin,xmax)  ((x) < (xmin) ? (xmin) : (x) > (xmax) ? (xmax) : (x))

static std::vector<float>* s_pSampleVec;

float* openOggFileAndReadPcmFramesF32(const char* filename, unsigned int* channels,
                                      unsigned int* sampleRate, uint64_t* totalFrameCount)
{
   *channels = 0;
   *sampleRate = 0;
   *totalFrameCount = 0;

   int error;
   stb_vorbis* v = stb_vorbis_open_filename(filename, &error, NULL);
   if (!v) {
      return NULL;
   }

   *channels = v->channels;
   *sampleRate = v->sample_rate;

   s_pSampleVec = new std::vector<float>;
   s_pSampleVec->reserve(10 * v->sample_rate * v->channels);  // reserve 10 seconds of space

   for(;;) {
      float** outputs;
      int num_c;
      int n = stb_vorbis_get_frame_float(v, &num_c, &outputs);
      for (int s = 0; s < n; s++) {
         for (int c = 0; c < num_c; c++) {
            s_pSampleVec->push_back(outputs[c][s]);
         }
      }
      *totalFrameCount += n;
      if (n == 0) {
         break;
      }
   }

   return s_pSampleVec->data();
}

void freeOggSampleData(float* pSampleData)
{
   if (s_pSampleVec != NULL &&
       pSampleData == s_pSampleVec->data()) {
      delete s_pSampleVec;
      s_pSampleVec = NULL;
   }
}
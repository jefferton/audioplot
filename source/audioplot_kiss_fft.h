#ifndef AUDIOPLOT_KISS_FFT_H
#define AUDIOPLOT_KISS_FFT_H

#include <vector>

class Spectrogram
{
public:
    Spectrogram();
    ~Spectrogram();

    void initialize(const std::vector<std::vector<double>>& samples, float sampleRate);

    const std::vector<float>& data(size_t ch) const;
    int n_frq() const;
    int n_bin() const;
    double min_db() const;
    double max_db() const;
    float min_frq() const;
    float max_frq() const;

private:
    class SpectrogramImpl;
    SpectrogramImpl* m_pImpl;
};

#endif // AUDIOPLOT_KISS_FFT_H

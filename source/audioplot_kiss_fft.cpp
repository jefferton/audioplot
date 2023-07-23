#include "audioplot_kiss_fft.h"

#include <kiss_fftr.h>
#include <complex>
#include <vector>
#include <array>

class Spectrogram::SpectrogramImpl
{
public:
    void initialize(const std::vector<std::vector<double>>& samples, float sampleRate)
    {
        for (int f = 0; f < (int)m_fft_frq.size(); ++f) {
            m_fft_frq[f] = f * sampleRate / (float)N_FFT;
        }

        m_channels.reserve(samples.size());    
        for (size_t ch = 0; ch < samples.size(); ch++) {
            m_channels.emplace_back(samples[ch]);
        }
    }

    const std::vector<float>& data(size_t ch) const
    { 
        return m_channels[ch].m_spectrogram;
    }

    int n_frq() const
    {
        return N_FRQ;
    }

    int n_bin() const
    {
        return m_channels[0].m_fft_bins;
    }

    double min_db() const
    {
        return m_min_db;
    }

    double max_db() const
    {
        return m_max_db;
    }

    float min_frq() const
    {
        return m_fft_frq.front();
    }

    float max_frq() const
    {
        return m_fft_frq.back();
    }

private:
    static constexpr int N_FFT = 1024;           // FFT size
    static constexpr int N_FRQ = N_FFT / 2 + 1;  // FFT frequency count
    static constexpr double m_min_db = -25;      // minimum spectrogram dB
    static constexpr double m_max_db =  40;      // maximum spectrogram dB
    std::array<float, N_FRQ> m_fft_frq;          // FFT output frequencies

    struct Channel
    {
        Channel(const std::vector<double>& samples)
        {
            // convert samples to float
            std::vector<float> float_samples;
            float_samples.reserve(samples.size());
            for (size_t s = 0; s < samples.size(); s++) {
                float_samples.push_back((float)samples[s]);
            }

            // compute FFTs to create a spectrogram  
            kiss_fftr_cfg fft = kiss_fftr_alloc(N_FFT, 0, nullptr, nullptr);
            m_fft_bins = samples.size() / N_FFT;
            m_spectrogram.resize(N_FRQ * m_fft_bins);
            std::complex<float> fft_out[N_FFT];
            int idx = 0;
            for (int b = 0; b < m_fft_bins; ++b) {
                kiss_fftr(fft, &float_samples[idx], reinterpret_cast<kiss_fft_cpx*>(fft_out));
                for (int f = 0; f < N_FRQ; ++f) {
                    m_spectrogram[f*m_fft_bins+b] = 20*log10f(std::abs(fft_out[N_FRQ-1-f]));
                }
                idx += N_FFT;
            }
            kiss_fftr_free(fft);
        }

        int m_fft_bins = 0; // spectrogram bin count
        std::vector<float>  m_spectrogram; // spectrogram matrix data
    };

    std::vector<Channel> m_channels;
};

Spectrogram::Spectrogram()
: m_pImpl(new Spectrogram::SpectrogramImpl())
{
}

Spectrogram::~Spectrogram()
{
    delete m_pImpl;
    m_pImpl = nullptr;
}

void Spectrogram::initialize(const std::vector<std::vector<double>>& samples, float sampleRate)
{
    m_pImpl->initialize(samples, sampleRate);
}

const std::vector<float>& Spectrogram::data(size_t ch) const
{ 
    return m_pImpl->data(ch); 
}

int Spectrogram::n_frq() const
{
    return m_pImpl->n_frq();
}

int Spectrogram::n_bin() const
{
    return m_pImpl->n_bin();
}

double Spectrogram::min_db() const
{
    return m_pImpl->min_db();
}

double Spectrogram::max_db() const
{
    return m_pImpl->max_db();
}

float Spectrogram::min_frq() const
{
    return m_pImpl->min_frq();
}

float Spectrogram::max_frq() const
{
    return m_pImpl->max_frq();
}


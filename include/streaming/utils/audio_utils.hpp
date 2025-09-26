// include/streaming/utils/audio_utils.hpp
#pragma once

#include <vector>
#include <cstdint>
#include <cmath>

namespace streaming {
namespace audio {

class AudioUtils {
public:
    // PCM format conversions
    static void float_to_pcm16(const float* input, int16_t* output, size_t samples);
    static void pcm16_to_float(const int16_t* input, float* output, size_t samples);
    
    // Audio processing
    static void apply_gain(int16_t* samples, size_t count, float gain_db);
    static void apply_limiter(int16_t* samples, size_t count, int16_t threshold);
    static void mix_stereo_to_mono(const int16_t* stereo, int16_t* mono, size_t samples);
    static void split_mono_to_stereo(const int16_t* mono, int16_t* stereo, size_t samples);
    
    // Psychoacoustic model helpers
    static void calculate_energy(const int16_t* samples, size_t count, 
                                std::vector<double>& energy_bands);
    static void apply_hanning_window(float* samples, size_t count);
    static void calculate_spectrum(const float* time_domain, 
                                  std::vector<float>& frequency_domain, size_t fft_size);
    
    // Quality metrics
    static double calculate_snr(const int16_t* original, const int16_t* decoded, size_t samples);
    static double calculate_spectral_flatness(const float* spectrum, size_t bins);
};

class Resampler {
public:
    Resampler();
    ~Resampler();
    
    bool initialize(uint32_t input_rate, uint32_t output_rate, uint16_t channels);
    bool resample(const int16_t* input, size_t input_samples, 
                 std::vector<int16_t>& output);
    void reset();

private:
    void* state_ = nullptr;
    uint32_t input_rate_ = 0;
    uint32_t output_rate_ = 0;
    uint16_t channels_ = 0;
};

} // namespace audio
} // namespace streaming
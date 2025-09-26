// include/streaming/audio/aac_encoder.hpp
#pragma once

#include "audio_codec.hpp"
#include <vector>
#include <memory>

namespace streaming {
namespace audio {

class AACEncoder : public IAudioEncoder {
private:
    struct AACFrame {
        std::vector<uint8_t> data;
        uint32_t sample_rate;
        uint16_t channels;
    };

public:
    AACEncoder();
    ~AACEncoder() override;
    
    bool initialize(const AudioConfig& config) override;
    bool encode_frame(const AudioFrame& input, std::vector<uint8_t>& output) override;
    bool encode_frames(const std::vector<AudioFrame>& inputs, 
                      std::vector<std::vector<uint8_t>>& outputs) override;
    void set_bitrate(uint32_t bitrate) override;
    uint32_t get_encoded_size() const override;
    double get_compression_ratio() const override;

    // AAC-specific features
    void set_aot(int aot); // Audio Object Type (LC, HE, HEv2)
    void set_profile(int profile); // AAC profile
    void enable_sbr(bool enable); // Spectral Band Replication
    void enable_ps(bool enable);  // Parametric Stereo

private:
    bool encode_aac_frame(const int16_t* samples, uint32_t frame_size, 
                         std::vector<uint8_t>& output);
    void apply_mdct_transform(std::vector<float>& time_domain, 
                             std::vector<float>& frequency_domain);
    void psychoacoustic_model_analysis(const std::vector<float>& frequency_domain,
                                      std::vector<float>& masking_threshold);
    
private:
    void* encoder_ = nullptr;
    AudioConfig config_;
    uint32_t total_encoded_bytes_ = 0;
    uint32_t total_input_samples_ = 0;
    
    // AAC specific
    int aot_ = 2; // AAC-LC by default
    bool sbr_enabled_ = false; // Spectral Band Replication
    bool ps_enabled_ = false;  // Parametric Stereo
    
    // Psychoacoustic model state
    std::vector<float> previous_frame_;
    std::vector<float> bark_scale_energies_;
};

} // namespace audio
} // namespace streaming
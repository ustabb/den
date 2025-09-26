// include/streaming/audio/opus_encoder.hpp
#pragma once

#include "audio_codec.hpp"
#include <vector>
#include <memory>

namespace streaming {
namespace audio {

class OpusEncoder : public IAudioEncoder {
private:
    struct OpusFrame {
        std::vector<uint8_t> data;
        uint32_t frame_size;
        bool voiced; // Voice activity detection
    };

public:
    OpusEncoder();
    ~OpusEncoder() override;
    
    bool initialize(const AudioConfig& config) override;
    bool encode_frame(const AudioFrame& input, std::vector<uint8_t>& output) override;
    bool encode_frames(const std::vector<AudioFrame>& inputs, 
                      std::vector<std::vector<uint8_t>>& outputs) override;
    void set_bitrate(uint32_t bitrate) override;
    uint32_t get_encoded_size() const override;
    double get_compression_ratio() const override;

    // Opus-specific features
    void enable_dtx(bool enable);    // Discontinuous Transmission
    void enable_vbr(bool enable);    // Variable Bitrate
    void set_application(int app);   // VOIP, AUDIO, LOW_DELAY
    
    // Streaming optimizations
    void set_packet_loss_percentage(int percentage);
    void enable_fec(bool enable);    // Forward Error Correction

private:
    bool encode_opus_frame(const int16_t* samples, uint32_t frame_size, 
                          std::vector<uint8_t>& output);
    void apply_psychoacoustic_optimizations(const int16_t* samples, uint32_t count);
    bool detect_voice_activity(const int16_t* samples, uint32_t count);
    
private:
    void* encoder_ = nullptr;
    AudioConfig config_;
    uint32_t total_encoded_bytes_ = 0;
    uint32_t total_input_samples_ = 0;
    
    // Streaming optimizations
    bool dtx_enabled_ = true;    // Save bandwidth during silence
    bool vbr_enabled_ = true;    // Better quality at same bitrate
    bool fec_enabled_ = false;   // Error resilience
    int packet_loss_percentage_ = 0;
    
    // State tracking
    uint32_t consecutive_silence_frames_ = 0;
    double current_complexity_ = 1.0;
};

} // namespace audio
} // namespace streaming
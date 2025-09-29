#pragma once
#include "audio_codec.hpp"
#include <vector>

namespace streaming {
namespace audio {

class PCMEncoder : public IAudioEncoder {
public:
    PCMEncoder();
    ~PCMEncoder() override;
    bool initialize(const AudioConfig& config) override;
    bool encode_frame(const AudioFrame& input, std::vector<uint8_t>& output) override;
    bool encode_frames(const std::vector<AudioFrame>& inputs, std::vector<std::vector<uint8_t>>& outputs) override;
    void set_bitrate(uint32_t bitrate) override;
    uint32_t get_encoded_size() const override;
    double get_compression_ratio() const override;
private:
    AudioConfig config_;
    uint32_t total_encoded_bytes_ = 0;
    uint32_t total_input_samples_ = 0;
};

} // namespace audio
} // namespace streaming

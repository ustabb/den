#include "streaming/audio/aac_encoder.hpp"
#include <vector>
#include <cstring>
#include <spdlog/spdlog.h>

namespace streaming {
namespace audio {

AACEncoder::AACEncoder() {}
AACEncoder::~AACEncoder() {}

bool AACEncoder::initialize(const AudioConfig& config) {
    config_ = config;
    spdlog::info("AACEncoder initialized: {} Hz, {} ch, {} bps", config.sample_rate, config.channels, config.bitrate);
    return true;
}

bool AACEncoder::encode_frame(const AudioFrame& input, std::vector<uint8_t>& output) {
    // Stub: just copy PCM as dummy data
    output.resize(input.samples.size() * sizeof(int16_t));
    std::memcpy(output.data(), input.samples.data(), output.size());
    total_encoded_bytes_ += output.size();
    total_input_samples_ += input.samples.size();
    return true;
}

bool AACEncoder::encode_frames(const std::vector<AudioFrame>& inputs, std::vector<std::vector<uint8_t>>& outputs) {
    outputs.clear();
    for (const auto& frame : inputs) {
        std::vector<uint8_t> out;
        encode_frame(frame, out);
        outputs.push_back(std::move(out));
    }
    return true;
}

void AACEncoder::set_bitrate(uint32_t bitrate) {
    config_.bitrate = bitrate;
}

uint32_t AACEncoder::get_encoded_size() const {
    return total_encoded_bytes_;
}

double AACEncoder::get_compression_ratio() const {
    return 1.0; // Stub
}

void AACEncoder::set_aot(int aot) { aot_ = aot; }
void AACEncoder::set_profile(int profile) { /* stub */ }
void AACEncoder::enable_sbr(bool enable) { sbr_enabled_ = enable; }
void AACEncoder::enable_ps(bool enable) { ps_enabled_ = enable; }

} // namespace audio
} // namespace streaming

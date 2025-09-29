#include "streaming/audio/opus_encoder.hpp"
#include <vector>
#include <cstring>
#include <spdlog/spdlog.h>

namespace streaming {
namespace audio {

OpusEncoder::OpusEncoder() {}
OpusEncoder::~OpusEncoder() {}

bool OpusEncoder::initialize(const AudioConfig& config) {
    config_ = config;
    spdlog::info("OpusEncoder initialized: {} Hz, {} ch, {} bps", config.sample_rate, config.channels, config.bitrate);
    return true;
}

bool OpusEncoder::encode_frame(const AudioFrame& input, std::vector<uint8_t>& output) {
    // Stub: just copy PCM as dummy data
    output.resize(input.samples.size() * sizeof(int16_t));
    std::memcpy(output.data(), input.samples.data(), output.size());
    total_encoded_bytes_ += output.size();
    total_input_samples_ += input.samples.size();
    return true;
}

bool OpusEncoder::encode_frames(const std::vector<AudioFrame>& inputs, std::vector<std::vector<uint8_t>>& outputs) {
    outputs.clear();
    for (const auto& frame : inputs) {
        std::vector<uint8_t> out;
        encode_frame(frame, out);
        outputs.push_back(std::move(out));
    }
    return true;
}

void OpusEncoder::set_bitrate(uint32_t bitrate) {
    config_.bitrate = bitrate;
}

uint32_t OpusEncoder::get_encoded_size() const {
    return total_encoded_bytes_;
}

double OpusEncoder::get_compression_ratio() const {
    return 1.0; // Stub
}

void OpusEncoder::enable_dtx(bool enable) { dtx_enabled_ = enable; }
void OpusEncoder::enable_vbr(bool enable) { vbr_enabled_ = enable; }
void OpusEncoder::set_application(int app) { /* stub */ }
void OpusEncoder::set_packet_loss_percentage(int percentage) { packet_loss_percentage_ = percentage; }
void OpusEncoder::enable_fec(bool enable) { fec_enabled_ = enable; }

} // namespace audio
} // namespace streaming

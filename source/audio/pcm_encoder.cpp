#include "streaming/audio/audio_codec.hpp"
#include <vector>
#include <cstring>

namespace streaming {
namespace audio {

class PCMEncoder : public IAudioEncoder {
public:
    PCMEncoder() = default;
    ~PCMEncoder() override = default;

    bool initialize(const AudioConfig& config) override {
        config_ = config;
        return true;
    }
    bool encode_frame(const AudioFrame& input, std::vector<uint8_t>& output) override {
        output.resize(input.samples.size() * sizeof(int16_t));
        std::memcpy(output.data(), input.samples.data(), output.size());
        total_encoded_bytes_ += output.size();
        total_input_samples_ += input.samples.size();
        return true;
    }
    bool encode_frames(const std::vector<AudioFrame>& inputs, std::vector<std::vector<uint8_t>>& outputs) override {
        outputs.clear();
        for (const auto& frame : inputs) {
            std::vector<uint8_t> out;
            encode_frame(frame, out);
            outputs.push_back(std::move(out));
        }
        return true;
    }
    void set_bitrate(uint32_t bitrate) override {
        // PCM bitrate is fixed by sample rate and channels
    }
    uint32_t get_encoded_size() const override {
        return total_encoded_bytes_;
    }
    double get_compression_ratio() const override {
        return 1.0; // No compression
    }
private:
    AudioConfig config_;
    uint32_t total_encoded_bytes_ = 0;
    uint32_t total_input_samples_ = 0;
};

} // namespace audio
} // namespace streaming

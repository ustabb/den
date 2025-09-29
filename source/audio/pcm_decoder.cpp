#include "streaming/audio/audio_codec.hpp"
#include <vector>
#include <cstring>

namespace streaming {
namespace audio {

class PCMDecoder : public IAudioDecoder {
public:
    PCMDecoder() = default;
    ~PCMDecoder() override = default;

    bool initialize(const AudioConfig& config) override {
        config_ = config;
        return true;
    }
    bool decode_frame(const uint8_t* data, size_t size, AudioFrame& output) override {
        size_t sample_count = size / sizeof(int16_t);
        output.samples.resize(sample_count);
        std::memcpy(output.samples.data(), data, size);
        output.sample_rate = config_.sample_rate;
        output.channels = config_.channels;
        output.frame_size = static_cast<uint32_t>(sample_count / config_.channels);
        return true;
    }
    void reset() override {
        // No state to reset for PCM
    }
private:
    AudioConfig config_;
};

} // namespace audio
} // namespace streaming

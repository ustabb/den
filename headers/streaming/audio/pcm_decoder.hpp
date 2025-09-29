#pragma once
#include "audio_codec.hpp"
#include <vector>

namespace streaming {
namespace audio {

class PCMDecoder : public IAudioDecoder {
public:
    PCMDecoder();
    ~PCMDecoder() override;
    bool initialize(const AudioConfig& config) override;
    bool decode_frame(const uint8_t* data, size_t size, AudioFrame& output) override;
    void reset() override;
private:
    AudioConfig config_;
};

} // namespace audio
} // namespace streaming

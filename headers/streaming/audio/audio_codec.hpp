// include/streaming/audio/audio_codec.hpp
#pragma once

#include <cstdint>
#include <vector>
#include <memory>

namespace streaming {
namespace audio {

struct AudioFrame {
    std::vector<int16_t> samples;    // PCM samples
    uint32_t sample_rate;           // 48000, 44100, etc.
    uint16_t channels;              // 1=mono, 2=stereo
    uint32_t frame_size;            // Samples per channel
    uint64_t timestamp;             // Presentation timestamp
};

struct AudioConfig {
    uint32_t sample_rate = 48000;
    uint16_t channels = 2;
    uint32_t bitrate = 128000;      // 128 kbps
    uint32_t frame_size = 960;      // 20ms at 48kHz
    uint32_t complexity = 5;        // Encoding complexity
};

class IAudioEncoder {
public:
    virtual ~IAudioEncoder() = default;
    
    virtual bool initialize(const AudioConfig& config) = 0;
    virtual bool encode_frame(const AudioFrame& input, std::vector<uint8_t>& output) = 0;
    virtual bool encode_frames(const std::vector<AudioFrame>& inputs, 
                              std::vector<std::vector<uint8_t>>& outputs) = 0;
    virtual void set_bitrate(uint32_t bitrate) = 0;
    virtual uint32_t get_encoded_size() const = 0;
    virtual double get_compression_ratio() const = 0;
};

class IAudioDecoder {
public:
    virtual ~IAudioDecoder() = default;
    
    virtual bool initialize(const AudioConfig& config) = 0;
    virtual bool decode_frame(const uint8_t* data, size_t size, AudioFrame& output) = 0;
    virtual void reset() = 0;
};

} // namespace audio
} // namespace streaming
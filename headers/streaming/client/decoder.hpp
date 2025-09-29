// include/streaming/client/decoder.hpp
#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include "streaming/codec/video_codec.hpp"
#include "streaming/codec/audio_codec.hpp"

namespace streaming {
namespace client {

class Decoder {
public:
    struct DecoderConfig {
        bool hardware_acceleration = true;
        uint32_t max_concurrent_frames = 3;
        uint32_t max_decode_time_ms = 16; // 60fps target
        bool enable_parallel_decoding = true;
        uint32_t thread_count = 2;
    };

    struct DecodedFrame {
        std::vector<uint8_t> data; // RGB or YUV data
        uint32_t width;
        uint32_t height;
        uint64_t timestamp;
        bool is_keyframe;
        uint32_t format; // 0=YUV420, 1=RGB24, 2=RGBA
    };

    Decoder();
    ~Decoder();
    
    bool initialize(const DecoderConfig& config);
    void shutdown();
    
    bool decode_video(const uint8_t* encoded_data, size_t size, 
                     DecodedFrame& output_frame, uint64_t timestamp);
    bool decode_audio(const uint8_t* encoded_data, size_t size,
                     audio::AudioFrame& output_frame, uint64_t timestamp);
    
    // Performance optimization
    void flush_buffers();
    void reset();
    void set_max_decode_time(uint32_t max_time_ms);
    
    // Hardware acceleration
    bool initialize_hardware_decoder();
    bool is_hardware_accelerated() const;

private:
    bool decode_video_software(const uint8_t* data, size_t size, DecodedFrame& output);
    bool decode_video_hardware(const uint8_t* data, size_t size, DecodedFrame& output);
    
    bool decode_audio_software(const uint8_t* data, size_t size, audio::AudioFrame& output);
    bool decode_audio_hardware(const uint8_t* data, size_t size, audio::AudioFrame& output);
    
    DecoderConfig config_;
    
    // Video decoders
    std::unique_ptr<codec::IVideoDecoder> h264_decoder_;
    std::unique_ptr<codec::IVideoDecoder> h265_decoder_;
    std::unique_ptr<codec::IVideoDecoder> av1_decoder_;
    
    // Audio decoders
    std::unique_ptr<audio::IAudioDecoder> opus_decoder_;
    std::unique_ptr<audio::IAudioDecoder> aac_decoder_;
    
    // Hardware acceleration
    void* hardware_context_ = nullptr;
    bool hardware_initialized_ = false;
};

} // namespace client
} // namespace streaming
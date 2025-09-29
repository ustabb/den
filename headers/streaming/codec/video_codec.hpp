// include/streaming/codec/video_codec.hpp
#pragma once

#include "../engine/types.hpp"
#include <cstdint>
#include <vector>
#include <memory>

namespace streaming {
namespace codec {

struct VideoFrame {
    std::vector<uint8_t> data;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint64_t timestamp;
    bool keyframe = false;
};

class IVideoEncoder {
public:
    virtual ~IVideoEncoder() = default;
    
    virtual bool initialize(uint32_t width, uint32_t height, uint32_t fps, 
                           uint32_t bitrate) = 0;
    virtual bool encode_frame(const VideoFrame& input, std::vector<uint8_t>& output) = 0;
    virtual void set_bitrate(uint32_t bitrate) = 0;
    virtual void set_gop_size(uint32_t gop_size) = 0;
    virtual uint32_t get_encoded_size() const = 0;
};

class IVideoDecoder {
public:
    virtual ~IVideoDecoder() = default;
    
    virtual bool initialize() = 0;
    virtual bool decode_frame(const uint8_t* data, size_t size, VideoFrame& output) = 0;
    virtual void reset() = 0;
};

} // namespace codec
} // namespace streaming
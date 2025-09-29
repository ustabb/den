#pragma once

#include <vector>
#include <cstdint>
#include <map>
#include <string>

namespace media {

struct FrameMetadata {
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t stride;
    std::string format;
    std::map<std::string, std::string> additional_info;
    
    FrameMetadata() : width(0), height(0), channels(0), stride(0) {}
    FrameMetadata(uint32_t w, uint32_t h, uint32_t c, uint32_t s = 0, const std::string& f = "")
        : width(w), height(h), channels(c), stride(s), format(f) {}
};

class RawFrame {
public:
    std::vector<uint8_t> data;
    FrameMetadata meta;
    
    RawFrame();
    RawFrame(const std::vector<uint8_t>& d, const FrameMetadata& m);
    RawFrame(const std::vector<uint8_t>& d, uint32_t w, uint32_t h, uint32_t c, uint32_t s = 0, const std::string& f = "");
    
    static RawFrame from_vector(const std::vector<uint8_t>& vec, const FrameMetadata& m);
    static RawFrame from_vector(const std::vector<uint8_t>& vec, uint32_t w = 0, uint32_t h = 0, uint32_t c = 0);
    
    std::vector<uint8_t> to_vector() const;
    bool empty() const;
    size_t size() const;
    
    // Metadata accessors
    uint32_t get_width() const { return meta.width; }
    uint32_t get_height() const { return meta.height; }
    uint32_t get_channels() const { return meta.channels; }
    std::string get_format() const { return meta.format; }
};

} // namespace media
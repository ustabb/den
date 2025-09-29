#pragma once
#include <vector>
#include <cstdint>

namespace media {

// RGB to YUV conversion for raw frames
class ColorConverter {
public:
    // Converts RGB24 to YUV420 planar
    static std::vector<uint8_t> rgbToYuv420(const std::vector<uint8_t>& rgb, int width, int height);
    // Converts YUV420 planar to RGB24
    static std::vector<uint8_t> yuv420ToRgb(const std::vector<uint8_t>& yuv, int width, int height);
};

} // namespace media

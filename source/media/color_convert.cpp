#include "color_convert.hpp"
#include <algorithm>

namespace media {

// Converts RGB24 to YUV420 planar
std::vector<uint8_t> ColorConverter::rgbToYuv420(const std::vector<uint8_t>& rgb, int width, int height) {
    std::vector<uint8_t> yuv(width * height * 3 / 2, 0);
    int frameSize = width * height;
    int uIndex = frameSize;
    int vIndex = frameSize + frameSize / 4;
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            int rgbIdx = (j * width + i) * 3;
            uint8_t r = rgb[rgbIdx];
            uint8_t g = rgb[rgbIdx + 1];
            uint8_t b = rgb[rgbIdx + 2];
            int y = ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
            int u = ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
            int v = ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
            yuv[j * width + i] = std::clamp(y, 0, 255);
            if (j % 2 == 0 && i % 2 == 0) {
                yuv[uIndex++] = std::clamp(u, 0, 255);
                yuv[vIndex++] = std::clamp(v, 0, 255);
            }
        }
    }
    return yuv;
}

// Converts YUV420 planar to RGB24
std::vector<uint8_t> ColorConverter::yuv420ToRgb(const std::vector<uint8_t>& yuv, int width, int height) {
    std::vector<uint8_t> rgb(width * height * 3, 0);
    int frameSize = width * height;
    int uIndex = frameSize;
    int vIndex = frameSize + frameSize / 4;
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            int yIdx = j * width + i;
            int uvIdx = (j / 2) * (width / 2) + (i / 2);
            int y = std::max(0, (int)yuv[yIdx] - 16);
            int u = (int)yuv[uIndex + uvIdx] - 128;
            int v = (int)yuv[vIndex + uvIdx] - 128;
            int c = 298 * y;
            int r = (c + 409 * v + 128) >> 8;
            int g = (c - 100 * u - 208 * v + 128) >> 8;
            int b = (c + 516 * u + 128) >> 8;
            int rgbIdx = yIdx * 3;
            rgb[rgbIdx]     = std::clamp(r, 0, 255);
            rgb[rgbIdx + 1] = std::clamp(g, 0, 255);
            rgb[rgbIdx + 2] = std::clamp(b, 0, 255);
        }
    }
    return rgb;
}

} // namespace media

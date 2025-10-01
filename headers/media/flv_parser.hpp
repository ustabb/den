#pragma once
#include <cstdint>
#include <vector>
#include <string>

struct FlvTag {
    uint8_t tagType;      // 8 = audio, 9 = video, 18 = script
    uint32_t dataSize;
    uint32_t timestamp;
    std::vector<uint8_t> data;
};

class FlvParser {
public:
    bool parseHeader(const std::vector<uint8_t>& buffer);
    FlvTag parseTag(const std::vector<uint8_t>& buffer, size_t& offset);
};

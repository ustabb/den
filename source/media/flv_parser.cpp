#include "flv_parser.hpp"
#include <stdexcept>
#include <cstring>

bool FlvParser::parseHeader(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < 9) return false;
    if (buffer[0] != 'F' || buffer[1] != 'L' || buffer[2] != 'V') {
        throw std::runtime_error("Invalid FLV header");
    }
    return true;
}

FlvTag FlvParser::parseTag(const std::vector<uint8_t>& buffer, size_t& offset) {
    if (offset + 11 > buffer.size()) throw std::runtime_error("FLV buffer too small");

    FlvTag tag;
    tag.tagType = buffer[offset];
    tag.dataSize = (buffer[offset+1] << 16) | (buffer[offset+2] << 8) | buffer[offset+3];
    tag.timestamp = (buffer[offset+4] << 16) | (buffer[offset+5] << 8) | buffer[offset+6];
    tag.timestamp |= (buffer[offset+7] << 24); // extended timestamp

    offset += 11;
    if (offset + tag.dataSize > buffer.size()) throw std::runtime_error("FLV tag data overflow");

    tag.data.assign(buffer.begin() + offset, buffer.begin() + offset + tag.dataSize);
    offset += tag.dataSize + 4; // skip previous tag size

    return tag;
}

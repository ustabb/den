#include "media/encoder.hpp"

using namespace media;

std::vector<uint8_t> Encoder::encode(const RawFrame& frame) {
    return encode(frame.data);
}

std::vector<uint8_t> RLEEncoder::encode(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> encoded;
    if (data.empty()) return encoded;
    
    uint8_t current = data[0];
    uint8_t count = 1;
    
    for (size_t i = 1; i < data.size(); i++) {
        if (data[i] == current && count < 255) {
            count++;
        } else {
            encoded.push_back(count);
            encoded.push_back(current);
            current = data[i];
            count = 1;
        }
    }
    
    encoded.push_back(count);
    encoded.push_back(current);
    
    return encoded;
}
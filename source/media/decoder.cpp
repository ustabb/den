#include "media/decoder.hpp"

using namespace media;

RawFrame Decoder::decode_frame(const std::vector<uint8_t>& data) {
    auto decoded = decode(data);
    return RawFrame::from_vector(decoded);
}

std::vector<uint8_t> RLEDecoder::decode(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> decoded;
    
    for (size_t i = 0; i < data.size(); i += 2) {
        if (i + 1 >= data.size()) break;
        
        uint8_t count = data[i];
        uint8_t value = data[i + 1];
        
        for (uint8_t j = 0; j < count; j++) {
            decoded.push_back(value);
        }
    }
    
    return decoded;
}
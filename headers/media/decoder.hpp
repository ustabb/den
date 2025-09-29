#pragma once

#include <vector>
#include <cstdint>
#include "frame.hpp"

namespace media {

class Decoder {
public:
    virtual ~Decoder() = default;
    
    virtual std::vector<uint8_t> decode(const std::vector<uint8_t>& data) = 0;
    virtual RawFrame decode_frame(const std::vector<uint8_t>& data);
};

class RLEDecoder : public Decoder {
public:
    std::vector<uint8_t> decode(const std::vector<uint8_t>& data) override;
};

} // namespace media
#pragma once

#include <vector>
#include <cstdint>
#include "frame.hpp"

namespace media {

class Encoder {
public:
    virtual ~Encoder() = default;
    
    virtual std::vector<uint8_t> encode(const std::vector<uint8_t>& data) = 0;
    virtual std::vector<uint8_t> encode(const RawFrame& frame);
};

class RLEEncoder : public Encoder {
public:
    std::vector<uint8_t> encode(const std::vector<uint8_t>& data) override;
};

} // namespace media
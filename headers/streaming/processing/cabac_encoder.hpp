// include/streaming/processing/cabac_encoder.hpp
#pragma once

#include <vector>
#include <cstdint>

namespace streaming {
namespace processing {

class CABACEncoder {
private:
    struct ContextModel {
        uint8_t state = 63; // Initial state (0-63)
        uint8_t mps = 0;    // Most Probable Symbol (0 or 1)
    };

public:
    CABACEncoder();
    
    void init_encoder(std::vector<uint8_t>& output_buffer);
    void encode_bit(ContextModel& ctx, bool bit);
    void encode_bin(ContextModel& ctx, uint32_t bin, int max_bins);
    void encode_terminator();
    void flush_encoder();

    // HEVC-specific encoding functions
    void encode_ae(v) // Adaptive Encoding
    void encode_sao_type(ContextModel& ctx, uint8_t type);
    void encode_cu_split_flag(ContextModel& ctx, bool split_flag);

private:
    void encode_regular(ContextModel& ctx, bool bit);
    void encode_bypass(bool bit);
    void renorm_encoder();

private:
    std::vector<uint8_t>* output_;
    uint64_t low_ = 0;
    uint64_t range_ = 510;
    int num_buffered_bytes_ = 0;
    uint8_t buffered_byte_ = 0xFF;
    int num_bits_left_ = 23;
};

} // namespace processing
} // namespace streaming
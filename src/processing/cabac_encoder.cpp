// src/processing/cabac_encoder.cpp
#include "streaming/processing/cabac_encoder.hpp"

namespace streaming {
namespace processing {

CABACEncoder::CABACEncoder() {
    // Initialize context models for HEVC
}

void CABACEncoder::init_encoder(std::vector<uint8_t>& output_buffer) {
    output_ = &output_buffer;
    low_ = 0;
    range_ = 510;
    num_buffered_bytes_ = 0;
    buffered_byte_ = 0xFF;
    num_bits_left_ = 23;
}

void CABACEncoder::encode_bit(ContextModel& ctx, bool bit) {
    uint32_t lps = get_lps_range(ctx.state);
    uint32_t mps_range = range_ - lps;
    
    if (bit == ctx.mps) {
        range_ = mps_range;
        if (range_ < 256) {
            renorm_encoder();
        }
        // Update MPS state
        if (ctx.state == 0) ctx.mps = 1 - ctx.mps;
        ctx.state = next_state_mps[ctx.state];
    } else {
        low_ += mps_range;
        range_ = lps;
        if (range_ < 256) {
            renorm_encoder();
        }
        // Update LPS state
        ctx.state = next_state_lps[ctx.state];
    }
}

void CABACEncoder::renorm_encoder() {
    while (range_ < 256) {
        if (low_ < (0x1FF << 23)) {
            // Write out buffered byte
            if (buffered_byte_ != 0xFF) {
                output_->push_back(buffered_byte_);
            }
            buffered_byte_ = (low_ >> 31) & 0xFF;
            low_ = (low_ << 9) & 0x7FFFFFFFFF;
            num_bits_left_ -= 9;
        } else {
            low_ = (low_ - (0x1FF << 23)) << 9;
            num_bits_left_ -= 9;
            buffered_byte_++;
        }
        range_ <<= 1;
    }
}

void CABACEncoder::encode_terminator() {
    // Encode termination symbol
    ContextModel term_ctx;
    term_ctx.state = 63;
    
    for (int i = 0; i < 4; ++i) {
        encode_bit(term_ctx, 1);
    }
    
    flush_encoder();
}

void CABACEncoder::flush_encoder() {
    // Finish encoding and write remaining bits
    if (buffered_byte_ != 0xFF) {
        output_->push_back(buffered_byte_);
    }
    
    // Write final bytes
    uint64_t final_bits = low_ >> (32 - num_bits_left_);
    for (int i = 0; i < (num_bits_left_ + 7) / 8; ++i) {
        output_->push_back((final_bits >> (i * 8)) & 0xFF);
    }
}

// HEVC state transition tables
constexpr uint8_t next_state_mps[64] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 62, 63
};

constexpr uint8_t next_state_lps[64] = {
    0, 0, 1, 2, 2, 4, 4, 5, 6, 7, 8, 9, 9, 11, 11, 12,
    13, 13, 15, 15, 16, 16, 18, 18, 19, 19, 21, 21, 22, 22, 23, 24,
    24, 25, 26, 26, 27, 27, 28, 29, 29, 30, 30, 30, 31, 32, 32, 33,
    33, 33, 34, 34, 35, 35, 35, 36, 36, 36, 37, 37, 37, 38, 38, 63
};

} // namespace processing
} // namespace streaming
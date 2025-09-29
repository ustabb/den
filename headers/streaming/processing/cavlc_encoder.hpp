// include/streaming/processing/cavlc_encoder.hpp
#pragma once

#include "../utils/bitstream.hpp"
#include <array>
#include <vector>

namespace streaming {
namespace processing {

class CAVLCEncoder {
private:
    struct CoefficientInfo {
        int16_t value;
        int8_t run; // Zero run length
        bool significant;
    };

public:
    void encode_residual(utils::BitstreamWriter& writer, 
                        const std::array<std::array<int16_t, 8>, 8>& block) {
        // Convert 8x8 block to 4x4 sub-blocks (H.264 style)
        std::vector<CoefficientInfo> coeffs = zigzag_scan(block);
        encode_coefficients(writer, coeffs);
    }
    
private:
    std::vector<CoefficientInfo> zigzag_scan(const std::array<std::array<int16_t, 8>, 8>& block) {
        // Zigzag scan order for 8x8 block
        constexpr int ZIGZAG_8x8[64] = {
            0,  1,  8, 16,  9,  2,  3, 10,
            17, 24, 32, 25, 18, 11,  4,  5,
            12, 19, 26, 33, 40, 48, 41, 34,
            27, 20, 13,  6,  7, 14, 21, 28,
            35, 42, 49, 56, 57, 50, 43, 36,
            29, 22, 15, 23, 30, 37, 44, 51,
            58, 59, 52, 45, 38, 31, 39, 46,
            53, 60, 61, 54, 47, 55, 62, 63
        };
        
        std::vector<CoefficientInfo> coeffs;
        int zero_run = 0;
        
        for (int i = 0; i < 64; ++i) {
            int x = ZIGZAG_8x8[i] % 8;
            int y = ZIGZAG_8x8[i] / 8;
            int16_t coeff = block[y][x];
            
            if (coeff == 0) {
                zero_run++;
            } else {
                if (zero_run > 0) {
                    coeffs.push_back({0, static_cast<int8_t>(zero_run), false});
                    zero_run = 0;
                }
                coeffs.push_back({coeff, 0, true});
            }
        }
        
        // Trailing zeros
        if (zero_run > 0) {
            coeffs.push_back({0, static_cast<int8_t>(zero_run), false});
        }
        
        return coeffs;
    }
    
    void encode_coefficients(utils::BitstreamWriter& writer, 
                            const std::vector<CoefficientInfo>& coeffs) {
        if (coeffs.empty()) {
            writer.write_bits(0b1, 1); // No coefficients
            return;
        }
        
        // Encode total coefficients and trailing ones
        int total_coeffs = 0;
        int trailing_ones = 0;
        
        for (const auto& coeff : coeffs) {
            if (coeff.significant) {
                total_coeffs++;
                if (std::abs(coeff.value) == 1) {
                    trailing_ones++;
                    if (trailing_ones == 3) break;
                }
            }
        }
        
        trailing_ones = std::min(trailing_ones, 3);
        
        // Encode total coefficients and trailing ones
        writer.write_ue(total_coeffs);
        writer.write_ue(trailing_ones);
        
        // Encode trailing ones signs
        int ones_encoded = 0;
        for (const auto& coeff : coeffs) {
            if (coeff.significant && std::abs(coeff.value) == 1 && ones_encoded < trailing_ones) {
                writer.write_bit(coeff.value > 0 ? 0 : 1); // 0=positive, 1=negative
                ones_encoded++;
            }
        }
        
        // Encode levels (non-trailing ones)
        int levels_encoded = 0;
        for (const auto& coeff : coeffs) {
            if (coeff.significant && std::abs(coeff.value) > 1) {
                encode_level(writer, coeff.value);
                levels_encoded++;
            }
        }
        
        // Encode total zeros and run before
        encode_total_zeros(writer, coeffs, total_coeffs);
        encode_run_before(writer, coeffs);
    }
    
    void encode_level(utils::BitstreamWriter& writer, int16_t level) {
        bool positive = level > 0;
        uint32_t abs_level = std::abs(level);
        
        if (abs_level < 15) {
            writer.write_bits((abs_level - 1) << 1 | (positive ? 0 : 1), 
                            (abs_level < 8 ? abs_level : 15) + 1);
        } else {
            // Large level encoding
            writer.write_bits(0b1111, 4);
            encode_level(writer, level > 0 ? level - 14 : level + 14);
        }
    }
    
    void encode_total_zeros(utils::BitstreamWriter& writer, 
                           const std::vector<CoefficientInfo>& coeffs, int total_coeffs) {
        int total_zeros = 0;
        for (const auto& coeff : coeffs) {
            if (!coeff.significant) {
                total_zeros += coeff.run;
            }
        }
        
        writer.write_ue(total_zeros);
    }
    
    void encode_run_before(utils::BitstreamWriter& writer, 
                          const std::vector<CoefficientInfo>& coeffs) {
        int zeros_left = 0;
        for (const auto& coeff : coeffs) {
            if (!coeff.significant) {
                zeros_left += coeff.run;
            }
        }
        
        for (const auto& coeff : coeffs) {
            if (coeff.significant) {
                if (zeros_left > 0) {
                    writer.write_ue(std::min(coeff.run, zeros_left));
                    zeros_left -= coeff.run;
                }
            }
        }
    }
};

} // namespace processing
} // namespace streaming
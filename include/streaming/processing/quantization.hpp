// include/streaming/processing/quantization.hpp
#pragma once

#include <array>
#include <cmath>

namespace streaming {
namespace processing {

class Quantizer {
private:
    // Standard quantization matrices (H.264)
    static constexpr std::array<std::array<uint8_t, 8>, 8> QP_MATRIX = {{
        {16, 16, 16, 16, 16, 16, 16, 16},
        {16, 16, 16, 16, 16, 16, 16, 16},
        {16, 16, 16, 16, 16, 16, 16, 16},
        {16, 16, 16, 16, 16, 16, 16, 16},
        {16, 16, 16, 16, 16, 16, 16, 16},
        {16, 16, 16, 16, 16, 16, 16, 16},
        {16, 16, 16, 16, 16, 16, 16, 16},
        {16, 16, 16, 16, 16, 16, 16, 16}
    }};

public:
    void quantize_block(std::array<std::array<double, 8>, 8>& dct_coeffs, int qp) {
        double scale = get_quantization_scale(qp);
        
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
                double q_step = QP_MATRIX[i][j] * scale;
                dct_coeffs[i][j] = std::round(dct_coeffs[i][j] / q_step);
            }
        }
    }
    
    void dequantize_block(std::array<std::array<double, 8>, 8>& dct_coeffs, int qp) {
        double scale = get_quantization_scale(qp);
        
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
                double q_step = QP_MATRIX[i][j] * scale;
                dct_coeffs[i][j] = dct_coeffs[i][j] * q_step;
            }
        }
    }
    
private:
    double get_quantization_scale(int qp) const {
        // H.264 quantization scaling
        static constexpr double QP_SCALE[] = {
            0.625, 0.6875, 0.8125, 0.875, 1.0, 1.125, 1.25, 1.375,
            1.625, 1.75, 2.0, 2.25, 2.5, 2.75, 3.25, 3.5,
            4.0, 4.5, 5.0, 5.5, 6.5, 7.0, 8.0, 9.0,
            10.0, 11.0, 13.0, 14.0, 16.0, 18.0, 20.0, 22.0,
            26.0, 28.0, 32.0, 36.0, 40.0, 44.0, 52.0, 56.0,
            64.0, 72.0, 80.0, 88.0, 104.0, 112.0, 128.0, 144.0
        };
        return QP_SCALE[qp % 48];
    }
};

} // namespace processing
} // namespace streaming
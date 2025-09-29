// include/streaming/processing/dct_transform.hpp
#pragma once

#include <cmath>
#include <array>

namespace streaming {
namespace processing {

class DCT {
private:
    static constexpr int N = 8;
    std::array<std::array<double, N>, N> cos_table_;
    
public:
    DCT() {
        // Precompute cosine table for performance
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                cos_table_[i][j] = std::cos((2 * i + 1) * j * M_PI / (2.0 * N));
            }
        }
    }
    
    void forward_dct(const std::array<std::array<int16_t, N>, N>& input,
                    std::array<std::array<double, N>, N>& output) {
        for (int u = 0; u < N; ++u) {
            for (int v = 0; v < N; ++v) {
                double sum = 0.0;
                
                for (int x = 0; x < N; ++x) {
                    for (int y = 0; y < N; ++y) {
                        sum += input[x][y] * cos_table_[x][u] * cos_table_[y][v];
                    }
                }
                
                double cu = (u == 0) ? 1.0 / std::sqrt(2) : 1.0;
                double cv = (v == 0) ? 1.0 / std::sqrt(2) : 1.0;
                
                output[u][v] = 0.25 * cu * cv * sum;
            }
        }
    }
    
    void inverse_dct(const std::array<std::array<double, N>, N>& input,
                    std::array<std::array<int16_t, N>, N>& output) {
        for (int x = 0; x < N; ++x) {
            for (int y = 0; y < N; ++y) {
                double sum = 0.0;
                
                for (int u = 0; u < N; ++u) {
                    for (int v = 0; v < N; ++v) {
                        double cu = (u == 0) ? 1.0 / std::sqrt(2) : 1.0;
                        double cv = (v == 0) ? 1.0 / std::sqrt(2) : 1.0;
                        
                        sum += cu * cv * input[u][v] * cos_table_[x][u] * cos_table_[y][v];
                    }
                }
                
                output[x][y] = static_cast<int16_t>(std::round(0.25 * sum));
            }
        }
    }
};

} // namespace processing
} // namespace streaming
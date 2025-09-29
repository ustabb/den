// include/streaming/codec/hevc_structures.hpp
#pragma once

#include <cstdint>
#include <array>

namespace streaming {
namespace codec {

// HEVC uses Coding Tree Units (CTU) instead of macroblocks
struct CTU {
    static constexpr int MAX_CU_SIZE = 64;
    static constexpr int MIN_CU_SIZE = 8;
    
    std::array<std::array<int16_t, MAX_CU_SIZE>, MAX_CU_SIZE> y_samples;
    std::array<std::array<int16_t, MAX_CU_SIZE/2>, MAX_CU_SIZE/2> u_samples;
    std::array<std::array<int16_t, MAX_CU_SIZE/2>, MAX_CU_SIZE/2> v_samples;
};

// HEVC Prediction Units (PU)
struct PredictionUnit {
    enum class Type {
        INTRA_2Nx2N,
        INTER_2Nx2N,
        INTER_2NxN,
        INTER_Nx2N,
        INTER_2NxnU,
        INTER_2NxnD,
        INTER_nLx2N,
        INTER_nRx2N
    };
    
    Type type;
    int16_t mv_x, mv_y; // Motion vector
    uint8_t ref_idx;    // Reference index
};

// HEVC Transform Units (TU)
struct TransformUnit {
    std::array<std::array<int16_t, 32>, 32> coeffs; // Up to 32x32 transforms
    uint8_t transform_size;
};

} // namespace codec
} // namespace streaming
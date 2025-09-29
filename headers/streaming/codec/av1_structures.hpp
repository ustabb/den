// include/streaming/codec/av1_structures.hpp
#pragma once

#include <cstdint>
#include <array>
#include <vector>

namespace streaming {
namespace codec {

// AV1 Super Blocks (128x128) - H.265'ten daha büyük!
struct SuperBlock {
    static constexpr int SIZE = 128;
    std::array<std::array<int16_t, SIZE>, SIZE> y_plane;
    std::array<std::array<int16_t, SIZE/2>, SIZE/2> u_plane;
    std::array<std::array<int16_t, SIZE/2>, SIZE/2> v_plane;
};

// AV1'in gelişmiş prediction modları
enum class PredictionMode {
    DC_PRED,        // DC prediction
    V_PRED,         // Vertical
    H_PRED,         // Horizontal
    D45_PRED,       // Diagonal 45
    D135_PRED,      // Diagonal 135
    D113_PRED,      // Diagonal 113
    D157_PRED,      // Diagonal 157
    SMOOTH_PRED,    // Smooth
    SMOOTH_V_PRED,  // Smooth Vertical
    SMOOTH_H_PRED,  // Smooth Horizontal
    PAETH_PRED,     // Paeth predictor
    // Intra inter prediction modları
    NEARESTMV, NEWMV, NEARMV, GLOBALMV
};

// AV1 Transform Blocks - çok daha esnek boyutlar
struct TransformBlock {
    std::vector<std::vector<int16_t>> coeffs;
    uint8_t tx_size;  // 4x4, 8x8, 16x16, 32x32, 64x64
    uint8_t tx_type;  // DCT, ADST, FLIPADST, IDTX
};

// AV1'in ünlü partitioning özelliği
enum class PartitionType {
    PARTITION_NONE,      // No partition
    PARTITION_HORZ,      // Horizontal split
    PARTITION_VERT,      // Vertical split
    PARTITION_SPLIT,     // 4-way split
    PARTITION_HORZ_A,    // Horizontal split (A)
    PARTITION_HORZ_B,    // Horizontal split (B)
    PARTITION_VERT_A,    // Vertical split (A)
    PARTITION_VERT_B,    // Vertical split (B)
    PARTITION_HORZ_4,    // 4-way horizontal
    PARTITION_VERT_4     // 4-way vertical
};

} // namespace codec
} // namespace streaming
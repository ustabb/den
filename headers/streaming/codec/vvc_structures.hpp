// include/streaming/codec/vvc_structures.hpp
#pragma once

#include <cstdint>
#include <array>
#include <vector>

namespace streaming {
namespace codec {

// VVC Coding Tree Units - 256x256'a kadar! (AV1'den 2x daha büyük)
struct VVCCTU {
    static constexpr int MAX_SIZE = 256;
    std::array<std::array<int16_t, MAX_SIZE>, MAX_SIZE> y_samples;
    std::array<std::array<int16_t, MAX_SIZE/2>, MAX_SIZE/2> u_samples;
    std::array<std::array<int16_t, MAX_SIZE/2>, MAX_SIZE/2> v_samples;
};

// VVC'nin Multi-Type Tree (MTT) partitioning'i
enum class VVCPartitionType {
    QT_SPLIT,          // Quadtree split (2Nx2N -> NxN)
    BT_HORZ_SPLIT,     // Binary tree horizontal (2NxN -> NxN)
    BT_VERT_SPLIT,     // Binary tree vertical (Nx2N -> NxN)
    TT_HORZ_SPLIT,     // Ternary tree horizontal (2NxN -> N/2xN)
    TT_VERT_SPLIT,     // Ternary tree vertical (Nx2N -> NxN/2)
    NO_SPLIT           // No further splitting
};

// VVC'nin gelişmiş prediction modları
enum class VVCPredictionMode {
    INTRA_PLANAR, INTRA_DC, INTRA_ANGULAR,
    INTER_MERGE, INTER_AMVP, AFFINE_INTER,
    IBC,            // Intra Block Copy (yeni!)
    CIIP,           // Combined Inter-Intra Prediction
    GPM             // Geometric Partition Mode (çok yeni!)
};

// VVC Transform - DCT-II, DST-VII, LGT (yeni!)
struct VVCTransformUnit {
    std::array<std::array<int16_t, 64>, 64> coeffs; // 64x64'e kadar
    uint8_t tr_type;    // Transform type
    uint8_t tr_size;    // Transform size
    bool mts_enabled;   // Multiple Transform Selection
};

// VVC'nin yeni özellikleri
struct VVCAdvancedFeatures {
    bool bdpcm_enabled;          // Block DPCM
    bool mip_enabled;            // Matrix-based Intra Prediction
    bool affine_enabled;         // Affine Motion Prediction
    bool smvd_enabled;           // Symmetric MVD
    bool prof_enabled;           // Prediction Refinement
    bool cclm_enabled;           // Cross-Component Linear Model
    bool ibc_enabled;            // Intra Block Copy
    bool palette_enabled;        // Palette Mode
    bool adaptive_resampling;    // Reference Picture Resampling
};

} // namespace codec
} // namespace streaming
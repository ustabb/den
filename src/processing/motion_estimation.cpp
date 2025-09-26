// src/processing/motion_estimation.cpp
#include "streaming/processing/motion_estimation.hpp"
#include <immintrin.h> // SIMD instructions

namespace streaming {
namespace processing {

uint16_t MotionEstimator::calculate_sad(const uint8_t* block1, const uint8_t* block2, int stride) const {
    uint16_t sad = 0;
    
    // Basic SAD calculation
    for (int y = 0; y < BLOCK_SIZE; ++y) {
        for (int x = 0; x < BLOCK_SIZE; ++x) {
            sad += std::abs(static_cast<int16_t>(block1[y * stride + x]) - 
                           static_cast<int16_t>(block2[y * stride + x]));
        }
    }
    
    return sad;
}

#ifdef __SSE2__
// SIMD-accelerated SAD calculation
uint16_t MotionEstimator::calculate_sad_simd(const uint8_t* block1, const uint8_t* block2, int stride) const {
    __m128i sad = _mm_setzero_si128();
    
    for (int y = 0; y < BLOCK_SIZE; ++y) {
        for (int x = 0; x < BLOCK_SIZE; x += 16) {
            __m128i src = _mm_load_si128(reinterpret_cast<const __m128i*>(block1 + y * stride + x));
            __m128i ref = _mm_load_si128(reinterpret_cast<const __m128i*>(block2 + y * stride + x));
            
            __m128i diff = _mm_sad_epu8(src, ref);
            sad = _mm_add_epi16(sad, diff);
        }
    }
    
    return _mm_extract_epi16(sad, 0) + _mm_extract_epi16(sad, 4);
}
#endif

uint16_t MotionEstimator::calculate_satd(const uint8_t* block1, const uint8_t* block2, int stride) const {
    // Sum of Absolute Transformed Differences - better for compression
    int32_t satd = 0;
    
    // Hadamard transform approximation
    for (int y = 0; y < BLOCK_SIZE; y += 4) {
        for (int x = 0; x < BLOCK_SIZE; x += 4) {
            // 4x4 sub-block SATD
            int16_t diff[4][4];
            
            // Calculate differences
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    diff[i][j] = static_cast<int16_t>(block1[(y + i) * stride + x + j]) - 
                                static_cast<int16_t>(block2[(y + i) * stride + x + j]);
                }
            }
            
            // Hadamard transform
            for (int i = 0; i < 4; ++i) {
                int16_t a = diff[i][0] + diff[i][2];
                int16_t b = diff[i][1] + diff[i][3];
                int16_t c = diff[i][0] - diff[i][2];
                int16_t d = diff[i][1] - diff[i][3];
                
                diff[i][0] = a + b;
                diff[i][1] = c + d;
                diff[i][2] = a - b;
                diff[i][3] = c - d;
            }
            
            // Vertical transform and accumulate
            for (int j = 0; j < 4; ++j) {
                int16_t a = diff[0][j] + diff[2][j];
                int16_t b = diff[1][j] + diff[3][j];
                int16_t c = diff[0][j] - diff[2][j];
                int16_t d = diff[1][j] - diff[3][j];
                
                satd += std::abs(a + b) + std::abs(c + d) + std::abs(a - b) + std::abs(c - d);
            }
        }
    }
    
    return static_cast<uint16_t>(satd / 2); // Normalization
}

uint16_t MotionEstimator::hybrid_cost(const uint8_t* block1, const uint8_t* block2, int stride, int mv_x, int mv_y) const {
    // Combine SAD with motion vector cost (rate-distortion optimization)
    uint16_t sad = calculate_sad(block1, block2, stride);
    
    // Motion vector cost (lambda * |MV|)
    int mv_cost = (std::abs(mv_x) + std::abs(mv_y)) * 2;
    
    return sad + mv_cost;
}

bool MotionEstimator::is_within_frame(int x, int y, int width, int height) const {
    return x >= 0 && y >= 0 && (x + BLOCK_SIZE) <= width && (y + BLOCK_SIZE) <= height;
}

MotionVector MotionEstimator::estimate_full_search(const uint8_t* current_frame, const uint8_t* reference_frame,
                                                  int width, int height, int x, int y) {
    MotionVector best_mv;
    const uint8_t* current_block = current_frame + y * width + x;
    
    // Search in [-SEARCH_RANGE, SEARCH_RANGE] range
    for (int dy = -SEARCH_RANGE; dy <= SEARCH_RANGE; ++dy) {
        for (int dx = -SEARCH_RANGE; dx <= SEARCH_RANGE; ++dx) {
            int ref_x = x + dx;
            int ref_y = y + dy;
            
            if (!is_within_frame(ref_x, ref_y, width, height)) continue;
            
            const uint8_t* ref_block = reference_frame + ref_y * width + ref_x;
            uint16_t cost = hybrid_cost(current_block, ref_block, width, dx, dy);
            
            if (cost < best_mv.cost) {
                best_mv = MotionVector(dx, dy, cost);
            }
            
            // Early termination
            if (best_mv.cost < EARLY_TERMINATION_THRESHOLD) {
                return best_mv;
            }
        }
    }
    
    return best_mv;
}

MotionVector MotionEstimator::estimate_diamond_search(const uint8_t* current_frame, const uint8_t* reference_frame,
                                                     int width, int height, int x, int y) {
    // Large Diamond Search Pattern (LDSP) points
    constexpr std::array<std::pair<int, int>, 9> ldsp = {{
        {0, 0}, {0, -4}, {0, 4}, {-4, 0}, {4, 0},
        {-2, -2}, {-2, 2}, {2, -2}, {2, 2}
    }};
    
    // Small Diamond Search Pattern (SDSP) points
    constexpr std::array<std::pair<int, int>, 5> sdsp = {{
        {0, 0}, {0, -1}, {0, 1}, {-1, 0}, {1, 0}
    }};
    
    MotionVector best_mv;
    const uint8_t* current_block = current_frame + y * width + x;
    
    // Step 1: LDSP until minimum is at center
    bool minimum_at_center = false;
    int center_x = 0, center_y = 0;
    
    while (!minimum_at_center) {
        minimum_at_center = true;
        
        for (const auto& [dx, dy] : ldsp) {
            int search_x = center_x + dx;
            int search_y = center_y + dy;
            int ref_x = x + search_x;
            int ref_y = y + search_y;
            
            if (!is_within_frame(ref_x, ref_y, width, height)) continue;
            
            const uint8_t* ref_block = reference_frame + ref_y * width + ref_x;
            uint16_t cost = hybrid_cost(current_block, ref_block, width, search_x, search_y);
            
            if (cost < best_mv.cost) {
                best_mv = MotionVector(search_x, search_y, cost);
                if (dx != 0 || dy != 0) { // Not center point
                    center_x = search_x;
                    center_y = search_y;
                    minimum_at_center = false;
                }
            }
        }
    }
    
    // Step 2: SDSP for refinement
    for (const auto& [dx, dy] : sdsp) {
        int search_x = center_x + dx;
        int search_y = center_y + dy;
        int ref_x = x + search_x;
        int ref_y = y + search_y;
        
        if (!is_within_frame(ref_x, ref_y, width, height)) continue;
        
        const uint8_t* ref_block = reference_frame + ref_y * width + ref_x;
        uint16_t cost = hybrid_cost(current_block, ref_block, width, search_x, search_y);
        
        if (cost < best_mv.cost) {
            best_mv = MotionVector(search_x, search_y, cost);
        }
    }
    
    return best_mv;
}

MotionVector MotionEstimator::estimate_three_step_search(const uint8_t* current_frame, const uint8_t* reference_frame,
                                                        int width, int height, int x, int y) {
    MotionVector best_mv;
    const uint8_t* current_block = current_frame + y * width + x;
    
    int step_size = 4; // Start with large step
    int center_x = 0, center_y = 0;
    
    // Three steps of search
    for (int step = 0; step < 3; ++step) {
        bool found_better = false;
        
        // Search 8 points around current center
        for (int dy = -step_size; dy <= step_size; dy += step_size) {
            for (int dx = -step_size; dx <= step_size; dx += step_size) {
                if (dx == 0 && dy == 0) continue; // Skip center (already checked)
                
                int search_x = center_x + dx;
                int search_y = center_y + dy;
                int ref_x = x + search_x;
                int ref_y = y + search_y;
                
                if (!is_within_frame(ref_x, ref_y, width, height)) continue;
                
                const uint8_t* ref_block = reference_frame + ref_y * width + ref_x;
                uint16_t cost = hybrid_cost(current_block, ref_block, width, search_x, search_y);
                
                if (cost < best_mv.cost) {
                    best_mv = MotionVector(search_x, search_y, cost);
                    center_x = search_x;
                    center_y = search_y;
                    found_better = true;
                }
            }
        }
        
        // Reduce step size for next iteration
        step_size /= 2;
        if (step_size < 1) step_size = 1;
        
        if (!found_better) break; // Early termination
    }
    
    return best_mv;
}

MotionVector MotionEstimator::estimate_adaptive(const uint8_t* current_frame, const uint8_t* reference_frame,
                                               int width, int height, int x, int y, int prev_mv_x, int prev_mv_y) {
    // Adaptive algorithm selection based on content characteristics
    
    // Check if previous motion vector is a good predictor
    if (prev_mv_x != 0 || prev_mv_y != 0) {
        int ref_x = x + prev_mv_x;
        int ref_y = y + prev_mv_y;
        
        if (is_within_frame(ref_x, ref_y, width, height)) {
            const uint8_t* current_block = current_frame + y * width + x;
            const uint8_t* ref_block = reference_frame + ref_y * width + ref_x;
            uint16_t cost = hybrid_cost(current_block, ref_block, width, prev_mv_x, prev_mv_y);
            
            // If previous MV is good enough, use it
            if (cost < EARLY_TERMINATION_THRESHOLD * 2) {
                return MotionVector(prev_mv_x, prev_mv_y, cost);
            }
        }
    }
    
    // Estimate scene complexity
    const uint8_t* current_block = current_frame + y * width + x;
    uint16_t variance = 0;
    uint16_t mean = 0;
    
    // Calculate block variance for complexity estimation
    for (int i = 0; i < BLOCK_SIZE * BLOCK_SIZE; ++i) {
        mean += current_block[i];
    }
    mean /= BLOCK_SIZE * BLOCK_SIZE;
    
    for (int i = 0; i < BLOCK_SIZE * BLOCK_SIZE; ++i) {
        variance += (current_block[i] - mean) * (current_block[i] - mean);
    }
    
    // Select algorithm based on complexity
    if (variance < 1000) { // Low complexity - fast search
        return estimate_three_step_search(current_frame, reference_frame, width, height, x, y);
    } else if (variance < 10000) { // Medium complexity
        return estimate_diamond_search(current_frame, reference_frame, width, height, x, y);
    } else { // High complexity - full search for best quality
        return estimate_full_search(current_frame, reference_frame, width, height, x, y);
    }
}

} // namespace processing
} // namespace streaming
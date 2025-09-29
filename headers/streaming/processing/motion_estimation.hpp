// include/streaming/processing/motion_estimation.hpp
#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

namespace streaming {
namespace processing {

struct MotionVector {
    int16_t x, y;
    uint16_t cost;
    bool valid = false;
    
    MotionVector() : x(0), y(0), cost(UINT16_MAX), valid(false) {}
    MotionVector(int16_t dx, int16_t dy, uint16_t c) : x(dx), y(dy), cost(c), valid(true) {}
};

class MotionEstimator {
private:
    static constexpr int BLOCK_SIZE = 16;
    static constexpr int SEARCH_RANGE = 32; // Pixels to search
    static constexpr int EARLY_TERMINATION_THRESHOLD = 256;
    
public:
    MotionEstimator() = default;
    
    // Full search motion estimation (accurate but slow)
    MotionVector estimate_full_search(const uint8_t* current_frame, const uint8_t* reference_frame,
                                     int width, int height, int x, int y);
    
    // Fast diamond search (good balance of speed/accuracy)
    MotionVector estimate_diamond_search(const uint8_t* current_frame, const uint8_t* reference_frame,
                                        int width, int height, int x, int y);
    
    // Three-step search (faster but less accurate)
    MotionVector estimate_three_step_search(const uint8_t* current_frame, const uint8_t* reference_frame,
                                           int width, int height, int x, int y);
    
    // Adaptive search based on content complexity
    MotionVector estimate_adaptive(const uint8_t* current_frame, const uint8_t* reference_frame,
                                  int width, int height, int x, int y, int prev_mv_x, int prev_mv_y);

private:
    uint16_t calculate_sad(const uint8_t* block1, const uint8_t* block2, int stride) const;
    uint16_t calculate_satd(const uint8_t* block1, const uint8_t* block2, int stride) const;
    bool is_within_frame(int x, int y, int width, int height) const;
    
    // Fast cost functions
    uint16_t hybrid_cost(const uint8_t* block1, const uint8_t* block2, int stride, int mv_x, int mv_y) const;
};

} // namespace processing
} // namespace streaming
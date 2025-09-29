// include/streaming/video/low_latency_encoder.hpp
#pragma once

#include "video_codec.hpp"
#include <atomic>

namespace streaming {
namespace video {

class LowLatencyEncoder {
public:
    struct LowLatencyConfig {
        uint32_t max_encoding_time_ms = 16;    // 60 FPS için 16ms
        uint32_t target_frame_size_ms = 8;     // Ultra-low latency
        bool enable_frame_dropping = true;
        bool enable_parallel_encoding = false; // Gecikme artırır!
        uint32_t lookahead_frames = 0;         // 0 for low latency
    };

    LowLatencyEncoder();
    
    bool initialize(const LowLatencyConfig& config);
    bool encode_frame_low_latency(const VideoFrame& input, 
                                 std::vector<uint8_t>& output,
                                 bool& frame_dropped);
    
    // Latency optimization methods
    void adaptive_quantization_control(double current_latency_ms);
    void dynamic_gop_adjustment(double network_latency_ms);
    void emergency_frame_drop(uint64_t frame_id);

private:
    bool should_drop_frame(const VideoFrame& frame, double encoding_time_estimate);
    void optimize_encoding_params_for_latency();
    void apply_low_latency_presets();
    
private:
    LowLatencyConfig config_;
    std::atomic<double> current_encoding_time_ms_{0};
    std::atomic<uint32_t> consecutive_dropped_frames_{0};
    std::chrono::high_resolution_clock::time_point last_frame_time_;
};

} // namespace video
} // namespace streaming
// src/video/low_latency_encoder.cpp
#include "streaming/video/low_latency_encoder.hpp"
#include <chrono>

namespace streaming {
namespace video {

LowLatencyEncoder::LowLatencyEncoder() {
    last_frame_time_ = std::chrono::high_resolution_clock::now();
}

bool LowLatencyEncoder::initialize(const LowLatencyConfig& config) {
    config_ = config;
    
    if (config_.max_encoding_time_ms < 5) {
        std::cerr << "⚠️ Warning: Very low encoding time may affect quality" << std::endl;
    }
    
    apply_low_latency_presets();
    
    std::cout << "🚀 LowLatencyEncoder initialized:" << std::endl;
    std::cout << "   Max encoding time: " << config_.max_encoding_time_ms << "ms" << std::endl;
    std::cout << "   Target frame size: " << config_.target_frame_size_ms << "ms" << std::endl;
    std::cout << "   Frame dropping: " << (config_.enable_frame_dropping ? "enabled" : "disabled") << std::endl;
    
    return true;
}

bool LowLatencyEncoder::encode_frame_low_latency(const VideoFrame& input, 
                                                std::vector<uint8_t>& output,
                                                bool& frame_dropped) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Check if we should drop this frame to maintain latency
    if (config_.enable_frame_dropping && 
        should_drop_frame(input, config_.max_encoding_time_ms)) {
        frame_dropped = true;
        consecutive_dropped_frames_++;
        return false;
    }
    
    frame_dropped = false;
    
    // Estimate encoding time based on frame complexity
    double estimated_time = estimate_encoding_time(input);
    if (estimated_time > config_.max_encoding_time_ms * 1.5) {
        // Frame too complex - apply aggressive optimization
        apply_emergency_optimizations(input);
    }
    
    // ACTUAL ENCODING WOULD HAPPEN HERE
    // Simulate encoding with controlled time
    std::this_thread::sleep_for(
        std::chrono::microseconds(static_cast<int>(config_.max_encoding_time_ms * 100))
    );
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto encoding_time = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time).count() / 1000.0;
    
    current_encoding_time_ms_ = encoding_time;
    
    // Update statistics
    if (encoding_time > config_.max_encoding_time_ms) {
        std::cout << "⚠️ Encoding time exceeded: " << encoding_time << "ms" << std::endl;
        adaptive_quantization_control(encoding_time);
    }
    
    return true;
}

bool LowLatencyEncoder::should_drop_frame(const VideoFrame& frame, double max_time_ms) {
    auto current_time = std::chrono::high_resolution_clock::now();
    auto time_since_last_frame = std::chrono::duration_cast<std::chrono::milliseconds>(
        current_time - last_frame_time_).count();
    
    // Drop frame if we're behind schedule
    if (time_since_last_frame > config_.target_frame_size_ms * 1.5) {
        return true;
    }
    
    // Drop frame if we've been dropping too many consecutively
    if (consecutive_dropped_frames_ > 5) {
        return false; // Force encode to maintain quality
    }
    
    // Estimate complexity-based dropping
    double complexity = estimate_frame_complexity(frame);
    double estimated_time = complexity * 0.1; // Empirical factor
    
    return estimated_time > max_time_ms;
}

void LowLatencyEncoder::adaptive_quantization_control(double current_latency_ms) {
    // Dynamically adjust QP based on latency
    double latency_ratio = current_latency_ms / config_.max_encoding_time_ms;
    
    if (latency_ratio > 2.0) {
        // Emergency mode - aggressive QP increase
        increase_qp(10);
    } else if (latency_ratio > 1.5) {
        increase_qp(5);
    } else if (latency_ratio < 0.5) {
        // We have extra time - improve quality
        decrease_qp(2);
    }
}

void LowLatencyEncoder::apply_low_latency_presets() {
    // Ultra-low latency optimizations
    config_.lookahead_frames = 0;          // No B-frames
    config_.enable_parallel_encoding = false; // Sequential processing
    
    // Encoding tools that increase latency
    disable_high_latency_features();
}

} // namespace video
} // namespace streaming
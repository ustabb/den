// include/streaming/engine/latency_analyzer.hpp
#pragma once

#include <chrono>
#include <vector>
#include <atomic>
#include <map>

namespace streaming {

struct LatencyBreakdown {
    // Video pipeline delays
    double frame_capture_ms = 0;
    double video_encoding_ms = 0;
    double packetization_ms = 0;
    
    // Audio pipeline delays  
    double audio_processing_ms = 0;
    double audio_encoding_ms = 0;
    
    // Network delays
    double network_buffering_ms = 0;
    double transmission_ms = 0;
    
    // Total latency
    double total_latency_ms = 0;
};

class LatencyAnalyzer {
private:
    struct TimingPoint {
        std::chrono::high_resolution_clock::time_point timestamp;
        std::string stage;
        uint64_t frame_id;
    };

public:
    LatencyAnalyzer();
    
    void mark_stage(const std::string& stage, uint64_t frame_id);
    LatencyBreakdown calculate_latency(uint64_t frame_id);
    void generate_latency_report();
    
    // Real-time latency monitoring
    double get_current_latency_ms() const;
    bool is_latency_within_bounds(double max_latency_ms) const;
    void trigger_latency_optimization();

private:
    std::vector<TimingPoint> timestamps_;
    mutable std::mutex mutex_;
    
    // Statistics
    std::atomic<double> avg_latency_ms_{0};
    std::atomic<double> max_latency_ms_{0};
    std::atomic<uint64_t> total_frames_{0};
};

} // namespace streaming
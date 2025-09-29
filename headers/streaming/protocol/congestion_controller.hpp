// include/streaming/protocol/congestion_controller.hpp
#pragma once

#include <cstdint>
#include <deque>
#include <atomic>
#include <cmath>

namespace streaming {
namespace protocol {

class CongestionController {
public:
    struct NetworkMetrics {
        uint32_t rtt_ms;              // Round-trip time
        uint32_t rtt_variance;        // RTT variance
        float packet_loss_rate;       // Packet loss percentage
        uint32_t available_bandwidth; // Estimated available bandwidth
        uint32_t send_rate;           // Current send rate
        uint32_t receive_rate;        // Current receive rate
    };

    struct CongestionState {
        enum State {
            SLOW_START,
            CONGESTION_AVOIDANCE,
            RECOVERY,
            FAST_RECOVERY
        };
        
        State current_state = SLOW_START;
        uint32_t congestion_window = 10;  // Packets
        uint32_t slow_start_threshold = UINT32_MAX;
        uint32_t bytes_in_flight = 0;
    };

    CongestionController();
    
    void update_metrics(const NetworkMetrics& metrics);
    uint32_t calculate_target_bitrate();
    uint32_t calculate_congestion_window();
    bool should_retransmit_packet(uint32_t packet_id, uint32_t timeout_ms);
    
    // Advanced algorithms
    void bbr_algorithm_update();
    void ledbat_algorithm_update();
    void pace_packet_transmission();

private:
    void tcp_reno_algorithm();
    void tcp_cubic_algorithm();
    void adaptive_bitrate_algorithm();
    
    double calculate_smooth_rtt() const;
    double calculate_loss_event_rate() const;
    uint32_t estimate_available_bandwidth() const;

private:
    NetworkMetrics current_metrics_;
    CongestionState state_;
    
    // History for trend analysis
    std::deque<uint32_t> rtt_history_;
    std::deque<float> loss_history_;
    std::deque<uint32_t> bandwidth_history_;
    
    // BBR-specific state
    uint32_t bbr_bottleneck_bandwidth_ = 0;
    uint32_t bbr_min_rtt_ = UINT32_MAX;
    uint32_t bbr_cycle_count_ = 0;
    
    // Timing
    uint64_t last_update_time_ = 0;
    uint32_t pacing_interval_us_ = 1000; // 1ms default
};

} // namespace protocol
} // namespace streaming
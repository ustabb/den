// src/network/low_latency_network.cpp
#include "streaming/network/socket_manager.hpp"

namespace streaming {
namespace network {

class LowLatencyNetworkManager {
public:
    struct NetworkLatencyConfig {
        uint32_t send_buffer_size = 0;          // No buffering
        uint32_t congestion_window = 2;         // Small CWND
        bool enable_nagle = false;              // Disable Nagle
        uint32_t max_retransmit_time_ms = 50;   // Quick retransmit
    };

    bool initialize(const NetworkLatencyConfig& config) {
        config_ = config;
        apply_network_optimizations();
        return true;
    }
    
    void apply_network_optimizations() {
        // TCP optimizations for low latency
        set_tcp_nodelay(true);
        set_tcp_quickack(true);
        reduce_socket_buffers();
        optimize_congestion_control();
    }
    
    bool send_immediate(const uint8_t* data, size_t size) {
        // Send immediately without buffering
        return send_with_priority(data, size, IPTOS_LOWDELAY);
    }

private:
    void optimize_congestion_control() {
        // Use low-latency congestion control
        #ifdef __linux__
        set_congestion_control("bbr");
        #endif
    }
};

} // namespace network
} // namespace streaming
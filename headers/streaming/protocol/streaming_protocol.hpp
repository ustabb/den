// include/streaming/protocol/streaming_protocol.hpp
#pragma once

#include "packet_format.hpp"
#include "../network/socket_manager.hpp"
#include <memory>
#include <queue>
#include <atomic>

namespace streaming {
namespace protocol {

class StreamingProtocol {
public:
    struct ProtocolConfig {
        uint32_t session_id = 0;
        uint32_t initial_bitrate = 1000000; // 1 Mbps
        uint32_t max_bitrate = 5000000;     // 5 Mbps
        uint32_t min_bitrate = 500000;      // 500 Kbps
        bool enable_fec = true;
        bool enable_retransmission = true;
        uint32_t fec_overhead = 10;         // 10% FEC overhead
        uint32_t max_latency_ms = 100;      // Maximum allowed latency
    };

    StreamingProtocol();
    ~StreamingProtocol();
    
    bool initialize(const ProtocolConfig& config);
    void shutdown();
    
    // Data transmission
    bool send_video_frame(const std::vector<uint8_t>& frame_data, 
                         FrameType frame_type, uint64_t timestamp);
    bool send_audio_frame(const std::vector<uint8_t>& audio_data, 
                         uint32_t sample_rate, uint16_t channels, uint64_t timestamp);
    
    // Session management
    bool start_session(const std::string& server_ip, uint16_t server_port);
    bool stop_session();
    bool reconnect_session();
    
    // Quality adaptation
    void adapt_to_network_conditions(float packet_loss, uint32_t rtt_ms, uint32_t available_bandwidth);
    void set_target_latency(uint32_t target_latency_ms);

    // Statistics
    struct ProtocolStats {
        uint64_t packets_sent = 0;
        uint64_t packets_received = 0;
        uint64_t packets_lost = 0;
        uint32_t current_bitrate = 0;
        uint32_t current_rtt = 0;
        float current_packet_loss = 0.0f;
        uint32_t queue_latency_ms = 0;
    };
    
    ProtocolStats get_statistics() const;

private:
    // Packet processing
    void packet_processing_loop();
    void network_processing_loop();
    void congestion_control_loop();
    
    // Packet creation
    std::vector<VideoPacket> create_video_packets(const std::vector<uint8_t>& frame_data,
                                                 FrameType frame_type, uint64_t timestamp);
    std::vector<AudioPacket> create_audio_packets(const std::vector<uint8_t>& audio_data,
                                                 uint32_t sample_rate, uint16_t channels, 
                                                 uint64_t timestamp);
    
    // Error protection
    void apply_fec_protection(std::vector<VideoPacket>& packets);
    void apply_retransmission_strategy(VideoPacket& packet);
    
    // Queue management
    bool add_to_send_queue(const ProtocolHeader& header, const std::vector<uint8_t>& payload);
    void manage_send_queue();

private:
    ProtocolConfig config_;
    std::unique_ptr<network::SocketManager> socket_manager_;
    
    // Threading
    std::atomic<bool> running_{false};
    std::thread packet_thread_;
    std::thread network_thread_;
    std::thread congestion_thread_;
    
    // Queues
    std::queue<std::vector<uint8_t>> send_queue_;
    std::queue<std::vector<uint8_t>> receive_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // State
    std::atomic<uint32_t> sequence_number_{0};
    std::atomic<uint32_t> current_bitrate_{0};
    std::atomic<uint32_t> current_rtt_{0};
    std::atomic<float> packet_loss_{0.0f};
    
    // Statistics
    mutable std::mutex stats_mutex_;
    ProtocolStats stats_;
};

} // namespace protocol
} // namespace streaming
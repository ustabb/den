// src/protocol/streaming_protocol.cpp
#include "streaming/protocol/streaming_protocol.hpp"
#include "streaming/protocol/session_manager.hpp"
#include "streaming/protocol/congestion_controller.hpp"
#include "streaming/protocol/fec_encoder.hpp"
#include "streaming/utils/logger.hpp"
#include <thread>
#include <chrono>

namespace streaming {
namespace protocol {

StreamingProtocol::StreamingProtocol() {
    socket_manager_ = std::make_unique<network::SocketManager>();
}

bool StreamingProtocol::initialize(const ProtocolConfig& config) {
    config_ = config;
    current_bitrate_ = config.initial_bitrate;
    
    if (!socket_manager_->initialize()) {
        LOG_ERROR("Failed to initialize socket manager");
        return false;
    }
    
    running_.store(true, std::memory_order_release);
    
    // Start processing threads
    packet_thread_ = std::thread([this]() { packet_processing_loop(); });
    network_thread_ = std::thread([this]() { network_processing_loop(); });
    congestion_thread_ = std::thread([this]() { congestion_control_loop(); });
    
    LOG_INFO("StreamingProtocol initialized with session ID: {}", config_.session_id);
    LOG_INFO("Initial bitrate: {} bps, Max latency: {} ms", 
             config_.initial_bitrate, config_.max_latency_ms);
    
    return true;
}

bool StreamingProtocol::send_video_frame(const std::vector<uint8_t>& frame_data, 
                                       FrameType frame_type, uint64_t timestamp) {
    if (!running_.load(std::memory_order_acquire)) {
        return false;
    }
    
    auto packets = create_video_packets(frame_data, frame_type, timestamp);
    
    if (config_.enable_fec) {
        apply_fec_protection(packets);
    }
    
    for (auto& packet : packets) {
        if (config_.enable_retransmission) {
            apply_retransmission_strategy(packet);
        }
        
        // Convert to wire format and queue
        std::vector<uint8_t> wire_packet = serialize_packet(packet);
        if (!add_to_send_queue(packet.header, wire_packet)) {
            LOG_WARN("Send queue full, dropping packet");
            return false;
        }
    }
    
    stats_.packets_sent += packets.size();
    return true;
}

std::vector<VideoPacket> StreamingProtocol::create_video_packets(
    const std::vector<uint8_t>& frame_data, FrameType frame_type, uint64_t timestamp) {
    
    std::vector<VideoPacket> packets;
    const size_t max_payload_size = constants::MAX_PAYLOAD_SIZE - sizeof(VideoPacket) + constants::HEADER_SIZE;
    
    size_t total_packets = (frame_data.size() + max_payload_size - 1) / max_payload_size;
    uint32_t frame_id = generate_frame_id();
    
    for (size_t i = 0; i < total_packets; ++i) {
        VideoPacket packet;
        
        // Initialize header
        packet.header.magic = constants::PROTOCOL_MAGIC;
        packet.header.version = constants::PROTOCOL_VERSION;
        packet.header.session_id = config_.session_id;
        packet.header.sequence_number = sequence_number_.fetch_add(1, std::memory_order_relaxed);
        packet.header.timestamp = timestamp;
        packet.header.packet_type = PacketType::VIDEO_DATA;
        packet.header.frame_type = frame_type;
        packet.header.flags = (i == 0) ? 0x01 : 0x00; // First packet flag
        packet.header.payload_size = 0; // Set after payload
        
        // Video-specific fields
        packet.frame_id = frame_id;
        packet.packet_index = static_cast<uint16_t>(i);
        packet.total_packets = static_cast<uint16_t>(total_packets);
        packet.fragment_offset = static_cast<uint32_t>(i * max_payload_size);
        
        // Copy payload
        size_t payload_size = std::min(max_payload_size, frame_data.size() - packet.fragment_offset);
        packet.payload.assign(frame_data.begin() + packet.fragment_offset,
                             frame_data.begin() + packet.fragment_offset + payload_size);
        
        packet.header.payload_size = static_cast<uint16_t>(payload_size);
        packet.header.header_checksum = calculate_header_checksum(packet.header);
        
        packets.push_back(packet);
    }
    
    return packets;
}

void StreamingProtocol::apply_fec_protection(std::vector<VideoPacket>& packets) {
    if (packets.size() <= 1) return; // FEC not needed for single packet
    
    FECEncoder fec_encoder;
    FECEncoder::FECConfig fec_config;
    
    // Adaptive FEC based on network conditions
    fec_config.data_packets = static_cast<uint16_t>(packets.size());
    fec_config.fec_packets = calculate_optimal_fec_count(packets.size());
    fec_config.adaptive_fec = true;
    
    if (fec_encoder.initialize(fec_config)) {
        auto fec_packets = fec_encoder.encode(packets);
        
        // Add FEC packets to the transmission
        for (auto& fec_packet : fec_packets) {
            std::vector<uint8_t> wire_packet = serialize_packet(fec_packet);
            add_to_send_queue(fec_packet.header, wire_packet);
        }
    }
}

void StreamingProtocol::packet_processing_loop() {
    LOG_INFO("Packet processing loop started");
    
    while (running_.load(std::memory_order_acquire)) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_cv_.wait(lock, [this]() { 
            return !send_queue_.empty() || !running_.load(std::memory_order_acquire); 
        });
        
        while (!send_queue_.empty()) {
            auto packet = std::move(send_queue_.front());
            send_queue_.pop();
            lock.unlock();
            
            // Send packet through network
            if (socket_manager_->is_connected()) {
                socket_manager_->send_data(packet.data(), packet.size());
            }
            
            lock.lock();
        }
    }
    
    LOG_INFO("Packet processing loop stopped");
}

void StreamingProtocol::congestion_control_loop() {
    LOG_INFO("Congestion control loop started");
    
    CongestionController congestion_ctl;
    
    while (running_.load(std::memory_order_acquire)) {
        // Update network metrics
        CongestionController::NetworkMetrics metrics;
        metrics.rtt_ms = current_rtt_.load(std::memory_order_acquire);
        metrics.packet_loss_rate = packet_loss_.load(std::memory_order_acquire);
        metrics.available_bandwidth = estimate_available_bandwidth();
        
        congestion_ctl.update_metrics(metrics);
        
        // Adjust bitrate based on congestion control
        uint32_t target_bitrate = congestion_ctl.calculate_target_bitrate();
        current_bitrate_.store(target_bitrate, std::memory_order_release);
        
        // Update statistics
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.current_bitrate = target_bitrate;
            stats_.current_rtt = metrics.rtt_ms;
            stats_.current_packet_loss = metrics.packet_loss_rate;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 10Hz update
    }
    
    LOG_INFO("Congestion control loop stopped");
}

} // namespace protocol
} // namespace streaming
// include/streaming/protocol/fec_encoder.hpp
#pragma once

#include "packet_format.hpp"
#include <vector>
#include <array>

namespace streaming {
namespace protocol {

class FECEncoder {
public:
    enum FECType {
        REED_SOLOMON = 0x01,
        XOR_BASED = 0x02,
        RAPTORQ = 0x03
    };

    struct FECConfig {
        FECType algorithm = XOR_BASED;
        uint16_t data_packets = 10;     // Number of data packets
        uint16_t fec_packets = 2;       // Number of FEC packets
        uint32_t symbol_size = 1024;    // Symbol size in bytes
        bool adaptive_fec = true;       // Adaptive based on network
    };

    FECEncoder();
    
    bool initialize(const FECConfig& config);
    std::vector<FECPacket> encode(const std::vector<VideoPacket>& data_packets);
    std::vector<VideoPacket> decode(const std::vector<VideoPacket>& received_packets,
                                   const std::vector<FECPacket>& fec_packets);
    
    // Adaptive FEC
    void adjust_fec_parameters(float packet_loss_rate, uint32_t rtt_ms);
    uint16_t calculate_optimal_fec_packets(uint16_t data_packets, float loss_rate);

private:
    // XOR-based FEC (simple and fast)
    std::vector<FECPacket> xor_encode(const std::vector<VideoPacket>& data_packets);
    std::vector<VideoPacket> xor_decode(const std::vector<VideoPacket>& received_packets,
                                       const std::vector<FECPacket>& fec_packets);
    
    // Reed-Solomon FEC (more robust)
    std::vector<FECPacket> reed_solomon_encode(const std::vector<VideoPacket>& data_packets);
    std::vector<VideoPacket> reed_solomon_decode(const std::vector<VideoPacket>& received_packets,
                                                const std::vector<FECPacket>& fec_packets);

private:
    FECConfig config_;
    uint16_t current_group_id_ = 0;
};

} // namespace protocol
} // namespace streaming
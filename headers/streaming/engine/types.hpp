#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace streaming {

struct StreamConfig {
    std::string host;
    uint16_t port;
    uint32_t max_packet_size = 1400;
    uint32_t batch_size = 10;
    uint32_t buffer_size_mb = 64;
    bool low_latency_mode = true;
    bool zero_copy_enabled = true;
};

class StreamPacket {
public:
    uint32_t sequence;
    uint64_t timestamp;
    std::vector<uint8_t> payload;
    std::string stream_id;
    
    StreamPacket(uint32_t seq, uint64_t ts, std::vector<uint8_t>&& data, const std::string& id)
        : sequence(seq), timestamp(ts), payload(std::move(data)), stream_id(id) {}
};

} // namespace streaming
// include/streaming/protocol/packet_format.hpp
#pragma once

#include <cstdint>
#include <vector>
#include <array>

namespace streaming {
namespace protocol {

// Packet Types
enum class PacketType : uint8_t {
    VIDEO_DATA = 0x10,
    AUDIO_DATA = 0x20,
    CONTROL = 0x30,
    FEC = 0x40,
    RETRANSMISSION = 0x50
};

// Video Frame Types
enum class FrameType : uint8_t {
    I_FRAME = 0x01,
    P_FRAME = 0x02,
    B_FRAME = 0x03,
    AUDIO_FRAME = 0x04
};

// Protocol Header (14 bytes - optimized for alignment)
struct ProtocolHeader {
    uint32_t magic;              // 0x5354524D "STRM"
    uint16_t version;            // Protocol version
    uint32_t session_id;         // Unique session identifier
    uint32_t sequence_number;    // Packet sequence number
    uint64_t timestamp;          // Microsecond timestamp
    PacketType packet_type;      // Packet type
    FrameType frame_type;        // Frame type
    uint8_t flags;               // Protocol flags
    uint16_t payload_size;       // Payload size
    uint16_t header_checksum;    // Header integrity check
};

// Video Packet Structure
struct VideoPacket {
    ProtocolHeader header;
    uint32_t frame_id;           // Video frame identifier
    uint16_t packet_index;       // Packet index in frame
    uint16_t total_packets;      // Total packets in frame
    uint32_t fragment_offset;    // Offset in original frame
    std::vector<uint8_t> payload;
};

// Audio Packet Structure  
struct AudioPacket {
    ProtocolHeader header;
    uint32_t sample_count;       // Number of audio samples
    uint32_t codec_timestamp;    // Codec-specific timestamp
    uint8_t audio_channels;      // Number of audio channels
    uint8_t audio_format;        // Audio format (0=PCM, 1=Opus, etc.)
    std::vector<uint8_t> payload;
};

// Control Packet Structure
struct ControlPacket {
    ProtocolHeader header;
    uint8_t control_type;        // Control message type
    uint32_t control_data;       // Control-specific data
    std::vector<uint8_t> extra_data;
};

// FEC Packet Structure
struct FECPacket {
    ProtocolHeader header;
    uint16_t fec_group_id;       // FEC group identifier
    uint8_t fec_type;            // FEC algorithm type
    uint16_t data_packets;       // Number of data packets
    uint16_t fec_packets;        // Number of FEC packets
    uint32_t protection_length;  // Protected data length
    std::vector<uint8_t> fec_data;
};

// Protocol Constants
namespace constants {
    constexpr uint32_t PROTOCOL_MAGIC = 0x5354524D; // "STRM"
    constexpr uint16_t PROTOCOL_VERSION = 0x0100;   // Version 1.0
    constexpr size_t MAX_PACKET_SIZE = 1400;        // MTU-friendly
    constexpr size_t HEADER_SIZE = sizeof(ProtocolHeader);
    constexpr size_t MAX_PAYLOAD_SIZE = MAX_PACKET_SIZE - HEADER_SIZE;
}

} // namespace protocol
} // namespace streaming
// include/streaming/client/demuxer.hpp
#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include "streaming/container/media_container.hpp"

namespace streaming {
namespace client {

class Demuxer {
public:
    struct DemuxedPacket {
        uint32_t track_id;
        uint64_t timestamp;
        uint64_t duration;
        bool is_keyframe;
        std::vector<uint8_t> data;
        container::TrackType track_type;
    };

    Demuxer();
    ~Demuxer();
    
    bool initialize(const std::string& format);
    bool open(const std::vector<uint8_t>& init_data);
    void close();
    
    DemuxedPacket read_packet();
    bool seek(uint64_t timestamp);
    
    // Stream information
    std::vector<container::TrackInfo> get_track_info() const;
    container::TrackInfo get_track_info(uint32_t track_id) const;
    uint64_t get_duration() const;
    
    // Format detection
    static std::string detect_format(const uint8_t* data, size_t size);
    static bool is_format_supported(const std::string& format);

private:
    bool parse_mp4_format(const uint8_t* data, size_t size, DemuxedPacket& packet);
    bool parse_webm_format(const uint8_t* data, size_t size, DemuxedPacket& packet);
    bool parse_flv_format(const uint8_t* data, size_t size, DemuxedPacket& packet);
    bool parse_hls_format(const uint8_t* data, size_t size, DemuxedPacket& packet);
    
    void build_sample_index();
    DemuxedPacket get_next_sample();

    std::unique_ptr<container::MediaContainer> container_;
    std::vector<container::TrackInfo> tracks_;
    uint64_t current_timestamp_ = 0;
    uint32_t current_packet_index_ = 0;
};

} // namespace client
} // namespace streaming
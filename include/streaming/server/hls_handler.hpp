// include/streaming/server/hls_handler.hpp
#pragma once

#include <boost/beast.hpp>
#include <vector>
#include <string>

namespace streaming {
namespace server {

class HlsHandler {
public:
    struct HlsConfig {
        uint32_t segment_duration = 2000; // 2 seconds
        uint32_t playlist_size = 3; // Number of segments in playlist
        std::vector<uint32_t> bitrate_levels = {500000, 1000000, 2000000}; // 500k, 1M, 2M
        std::string segment_template = "stream_$Bandwidth$_$Number$.ts";
        std::string playlist_template = "stream_$Bandwidth$.m3u8";
    };

    struct HlsSegment {
        uint32_t sequence_number;
        uint32_t bitrate;
        uint64_t start_time;
        uint64_t duration;
        std::string filename;
        std::vector<uint8_t> data;
    };

    HlsHandler();
    ~HlsHandler();
    
    bool handle_request(boost::beast::http::request<boost::beast::http::string_body>& request,
                       boost::beast::http::response<boost::beast::http::dynamic_body>& response);
    
    void create_segment(const std::string& stream_name, const uint8_t* data, 
                       uint32_t size, uint64_t timestamp, uint32_t bitrate);
    void update_playlist(const std::string& stream_name, uint32_t bitrate);
    void generate_master_playlist(const std::string& stream_name);

    // HLS playlist generation
    std::string generate_variant_playlist(const std::string& stream_name, uint32_t bitrate);
    std::string generate_master_playlist_content(const std::string& stream_name);

private:
    void encode_to_ts(const uint8_t* data, uint32_t size, uint32_t bitrate, 
                     std::vector<uint8_t>& ts_data);
    void write_segment_to_disk(const HlsSegment& segment);
    void cleanup_old_segments(const std::string& stream_name);

    HlsConfig config_;
    std::unordered_map<std::string, std::vector<HlsSegment>> segments_;
    std::unordered_map<std::string, uint32_t> sequence_counters_;
};

} // namespace server
} // namespace streaming
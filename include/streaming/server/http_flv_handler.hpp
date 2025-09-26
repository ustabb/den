// include/streaming/server/http_flv_handler.hpp
#pragma once

#include <boost/beast.hpp>
#include <memory>

namespace streaming {
namespace server {

class HttpFlvHandler {
public:
    struct FlvHeader {
        uint8_t signature[3] = {'F', 'L', 'V'};
        uint8_t version = 1;
        uint8_t flags = 5; // Audio + Video
        uint32_t data_offset = 9;
    };

    struct FlvTag {
        uint8_t tag_type;
        uint32_t data_size;
        uint32_t timestamp;
        uint32_t stream_id = 0;
        std::vector<uint8_t> data;
    };

    HttpFlvHandler();
    ~HttpFlvHandler();
    
    bool handle_request(boost::beast::http::request<boost::beast::http::string_body>& request,
                       boost::beast::http::response<boost::beast::http::dynamic_body>& response);
    
    void send_flv_header(boost::beast::tcp_stream& stream);
    void send_flv_tag(boost::beast::tcp_stream& stream, const FlvTag& tag);
    void send_metadata(boost::beast::tcp_stream& stream, const std::string& stream_name);

    // FLV packet creation
    FlvTag create_video_tag(const uint8_t* data, uint32_t size, uint32_t timestamp, bool is_keyframe);
    FlvTag create_audio_tag(const uint8_t* data, uint32_t size, uint32_t timestamp);
    FlvTag create_metadata_tag(const std::string& stream_name);

private:
    std::vector<uint8_t> build_flv_header();
    std::vector<uint8_t> build_flv_tag(const FlvTag& tag);
    
    // FLV specific constants
    static constexpr uint8_t AUDIO_TAG = 8;
    static constexpr uint8_t VIDEO_TAG = 9;
    static constexpr uint8_t SCRIPT_TAG = 18;
};

} // namespace server
} // namespace streaming
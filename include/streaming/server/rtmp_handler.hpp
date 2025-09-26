// include/streaming/server/rtmp_handler.hpp
#pragma once

#include <boost/asio.hpp>
#include <memory>

namespace streaming {
namespace server {

class RtmpHandler {
public:
    struct RtmpHeader {
        uint8_t format_type;
        uint32_t timestamp;
        uint32_t message_length;
        uint8_t message_type;
        uint32_t stream_id;
        uint32_t chunk_stream_id;
    };

    struct RtmpMessage {
        RtmpHeader header;
        std::vector<uint8_t> payload;
    };

    RtmpHandler();
    ~RtmpHandler();
    
    void handle_connection(boost::asio::ip::tcp::socket socket);
    bool handshake(boost::asio::ip::tcp::socket& socket);
    void process_message(const RtmpMessage& message, const std::string& stream_name);

    // RTMP message handlers
    void handle_connect(const RtmpMessage& message);
    void handle_publish(const RtmpMessage& message);
    void handle_play(const RtmpMessage& message);
    void handle_video_data(const RtmpMessage& message);
    void handle_audio_data(const RtmpMessage& message);
    void handle_metadata(const RtmpMessage& message);

    // RTMP message creation
    RtmpMessage create_window_ack_size(uint32_t size);
    RtmpMessage create_set_peer_bandwidth(uint32_t bandwidth);
    RtmpMessage create_stream_begin(uint32_t stream_id);
    RtmpMessage create_on_status(const std::string& code, const std::string& level, 
                                const std::string& description);

private:
    bool read_rtmp_message(boost::asio::ip::tcp::socket& socket, RtmpMessage& message);
    bool write_rtmp_message(boost::asio::ip::tcp::socket& socket, const RtmpMessage& message);
    
    void send_chunk(boost::asio::ip::tcp::socket& socket, const RtmpMessage& message, 
                   uint32_t chunk_size = 128);
    
    // RTMP state
    std::unordered_map<uint32_t, RtmpHeader> previous_headers_;
    uint32_t chunk_size_ = 128;
    uint32_t window_ack_size_ = 2500000;
};

} // namespace server
} // namespace streaming
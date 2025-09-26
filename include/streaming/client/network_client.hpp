// include/streaming/client/network_client.hpp
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <boost/asio.hpp>

namespace streaming {
namespace client {

class NetworkClient {
public:
    struct NetworkConfig {
        std::string server_url;
        uint32_t connection_timeout_ms = 5000;
        uint32_t read_timeout_ms = 10000;
        uint32_t buffer_size = 65536;
        uint32_t max_retries = 3;
        bool enable_http2 = true;
        bool enable_compression = true;
    };

    struct DownloadStats {
        uint64_t total_bytes_downloaded = 0;
        uint32_t current_bitrate = 0;
        uint32_t average_download_speed = 0;
        uint32_t packet_loss = 0;
        uint32_t network_latency = 0;
        uint32_t buffer_level_ms = 0;
    };

    NetworkClient();
    ~NetworkClient();
    
    bool initialize(const NetworkConfig& config);
    void shutdown();
    
    bool connect();
    void disconnect();
    bool is_connected() const;
    
    // Data transfer
    std::vector<uint8_t> download_data(size_t max_size);
    bool download_to_buffer(std::vector<uint8_t>& buffer, size_t expected_size);
    
    // Adaptive streaming
    void set_target_bitrate(uint32_t bitrate);
    void enable_adaptive_streaming(bool enable);
    void set_max_buffer_duration(uint32_t duration_ms);
    
    // Protocol handlers
    bool handle_http_streaming();
    bool handle_rtmp_streaming();
    bool handle_web_socket_streaming();
    bool handle_hls_streaming();

    DownloadStats get_statistics() const;

private:
    bool http_download_segment(const std::string& url, std::vector<uint8_t>& buffer);
    bool rtmp_handshake();
    bool websocket_handshake();
    
    void calculate_download_metrics();
    void adjust_download_strategy();

    NetworkConfig config_;
    boost::asio::io_context io_context_;
    std::unique_ptr<boost::asio::ip::tcp::socket> socket_;
    std::unique_ptr<boost::asio::ssl::context> ssl_context_;
    
    DownloadStats stats_;
    std::atomic<bool> connected_{false};
};

} // namespace client
} // namespace streaming
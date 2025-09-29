// include/streaming/server/streaming_server.hpp
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <boost/asio.hpp>

namespace streaming {
namespace server {

class StreamingServer {
public:
    struct ServerConfig {
        uint16_t http_port = 8080;
        uint16_t rtmp_port = 1935;
        uint16_t websocket_port = 8081;
        uint32_t max_connections = 1000;
        uint32_t worker_threads = 4;
        std::string document_root = "./www";
        uint32_t stream_timeout_ms = 30000;
        bool enable_hls = true;
        bool enable_http_flv = true;
        bool enable_rtmp = true;
    };

    StreamingServer();
    ~StreamingServer();
    
    bool initialize(const ServerConfig& config);
    void start();
    void stop();
    void wait_for_shutdown();

    // Stream management
    bool create_stream(const std::string& stream_name, const std::string& source_url = "");
    bool delete_stream(const std::string& stream_name);
    bool push_stream_data(const std::string& stream_name, const uint8_t* data, 
                         size_t size, uint64_t timestamp);
    
    // Client statistics
    struct ServerStats {
        uint32_t active_connections;
        uint32_t total_streams;
        uint64_t bytes_sent;
        uint64_t bytes_received;
        uint32_t active_sessions;
    };
    
    ServerStats get_statistics() const;

private:
    void start_http_server();
    void start_rtmp_server();
    void start_websocket_server();
    void cleanup_expired_sessions();
    
    // Protocol handlers
    void handle_http_request(boost::asio::ip::tcp::socket socket);
    void handle_rtmp_connection(boost::asio::ip::tcp::socket socket);
    void handle_websocket_connection(boost::asio::ip::tcp::socket socket);

private:
    ServerConfig config_;
    boost::asio::io_context io_context_;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> http_acceptor_;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> rtmp_acceptor_;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> websocket_acceptor_;
    
    std::atomic<bool> running_{false};
    std::vector<std::thread> worker_threads_;
    std::thread cleanup_thread_;
    
    // Managers
    std::unique_ptr<SessionHandler> session_handler_;
    std::unique_ptr<StreamManager> stream_manager_;
    
    // Protocol handlers
    std::unique_ptr<HttpFlvHandler> http_flv_handler_;
    std::unique_ptr<HlsHandler> hls_handler_;
    std::unique_ptr<RtmpHandler> rtmp_handler_;
};

} // namespace server
} // namespace streaming
// include/streaming/server/session_handler.hpp
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <boost/asio.hpp>

namespace streaming {
namespace server {

class StreamingSession {
public:
    enum class SessionType {
        HTTP_FLV,
        HLS,
        RTMP,
        WEBSOCKET,
        DASH
    };

    struct SessionInfo {
        std::string session_id;
        SessionType type;
        std::string stream_name;
        std::string client_ip;
        uint64_t start_time;
        uint64_t last_activity;
        uint64_t bytes_sent;
        uint64_t bytes_received;
        uint32_t packet_count;
        bool is_authenticated;
        bool is_active;
    };

    StreamingSession(boost::asio::ip::tcp::socket socket, SessionType type);
    ~StreamingSession();
    
    void start();
    void stop();
    void send_data(const uint8_t* data, size_t size);
    void handle_request(const std::vector<uint8_t>& request);
    
    // Session management
    bool authenticate(const std::string& token);
    void update_activity();
    bool is_expired(uint64_t timeout_ms) const;

private:
    void read_loop();
    void write_loop();
    void process_protocol_data(const std::vector<uint8_t>& data);
    
    boost::asio::ip::tcp::socket socket_;
    SessionInfo info_;
    std::vector<uint8_t> read_buffer_;
    std::queue<std::vector<uint8_t>> write_queue_;
    std::atomic<bool> active_{false};
};

class SessionHandler {
public:
    SessionHandler();
    ~SessionHandler();
    
    std::shared_ptr<StreamingSession> create_session(
        boost::asio::ip::tcp::socket socket, 
        StreamingSession::SessionType type);
    
    bool remove_session(const std::string& session_id);
    std::shared_ptr<StreamingSession> get_session(const std::string& session_id);
    
    void broadcast_to_sessions(const std::string& stream_name, 
                              const uint8_t* data, size_t size);
    void cleanup_expired_sessions(uint64_t timeout_ms = 30000);

    // Statistics
    uint32_t get_active_session_count() const;
    uint32_t get_session_count_by_type(StreamingSession::SessionType type) const;

private:
    std::unordered_map<std::string, std::shared_ptr<StreamingSession>> sessions_;
    mutable std::mutex sessions_mutex_;
    std::atomic<uint32_t> session_counter_{0};
};

} // namespace server
} // namespace streaming
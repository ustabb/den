// include/streaming/protocol/session_manager.hpp
#pragma once

#include "packet_format.hpp"
#include <memory>
#include <unordered_map>

namespace streaming {
namespace protocol {

class SessionManager {
public:
    struct SessionInfo {
        uint32_t session_id;
        std::string client_id;
        std::string server_address;
        uint16_t server_port;
        uint64_t start_time;
        uint64_t last_activity;
        uint32_t total_bytes_sent;
        uint32_t total_bytes_received;
        bool is_authenticated;
        bool is_encrypted;
    };

    SessionManager();
    ~SessionManager();
    
    bool create_session(uint32_t session_id, const std::string& server_ip, uint16_t port);
    bool authenticate_session(uint32_t session_id, const std::string& auth_token);
    bool close_session(uint32_t session_id);
    
    // Session state management
    void update_session_activity(uint32_t session_id);
    bool validate_session(uint32_t session_id);
    void cleanup_expired_sessions(uint64_t timeout_ms = 30000);
    
    // Multi-session support
    bool switch_session(uint32_t new_session_id);
    bool has_active_session() const;
    
    // Security
    bool enable_encryption(uint32_t session_id, const std::vector<uint8_t>& key);
    bool validate_packet_integrity(const ProtocolHeader& header, 
                                  const std::vector<uint8_t>& payload);

private:
    void handshake_procedure(uint32_t session_id);
    void keepalive_procedure();
    void session_recovery_procedure(uint32_t session_id);
    
    std::string generate_session_token(uint32_t session_id);
    bool verify_session_token(uint32_t session_id, const std::string& token);

private:
    std::unordered_map<uint32_t, SessionInfo> sessions_;
    uint32_t current_session_id_ = 0;
    mutable std::mutex sessions_mutex_;
    
    // Security
    std::vector<uint8_t> encryption_key_;
    bool encryption_enabled_ = false;
};

} // namespace protocol
} // namespace streaming
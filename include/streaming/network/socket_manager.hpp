#pragma once

#include "../engine/types.hpp"
#include "tcp_optimizer.hpp"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <netinet/in.h>

namespace streaming {
namespace network {

class ManagedSocket {
public:
    ManagedSocket(const std::string& host, uint16_t port);
    ~ManagedSocket();
    
    bool is_connected() const { return connected_; }
    ssize_t send(const uint8_t* data, size_t size);
    ssize_t receive(uint8_t* buffer, size_t size);
    void close();
    
private:
    int sockfd_ = -1;
    std::string host_;
    uint16_t port_;
    bool connected_ = false;
    sockaddr_in remote_addr_;
};

class SocketManager {
public:
    SocketManager();
    ~SocketManager();
    
    bool initialize(const StreamConfig& config);
    bool connect(const std::string& host, uint16_t port);
    ssize_t send(const std::string& host, uint16_t port, 
                const std::vector<uint8_t>& data);
    ssize_t receive(const std::string& host, uint16_t port,
                   std::vector<uint8_t>& buffer);
    
    void disconnect(const std::string& host, uint16_t port);
    void cleanup();
    
    size_t get_connection_count() const;
    bool is_connected(const std::string& host, uint16_t port) const;


    static SocketManager& get_instance();
    
    bool initialize(const StreamConfig& config);
    void shutdown();
    
    // Connection pooling
    std::shared_ptr<ManagedSocket> acquire_connection(const std::string& host, uint16_t port);
    void release_connection(const std::string& host, uint16_t port, 
                           std::shared_ptr<ManagedSocket> socket);
    
    // Direct streaming interface
    bool stream_video_data(const std::string& host, uint16_t port, 
                          const uint8_t* data, size_t size);

private:
    std::shared_ptr<ManagedSocket> get_connection(const std::string& host, uint16_t port);
    void return_connection(const std::string& host, uint16_t port, 
                          std::shared_ptr<ManagedSocket> socket);
    void health_check();

    SocketManager() = default;
    ~SocketManager() { shutdown(); }
    
    void health_check_loop();
    void cleanup_idle_connections();
    
    struct ConnectionPool {
        std::queue<std::shared_ptr<ManagedSocket>> available;
        size_t max_size = 10;
        size_t total_created = 0;
    };
    
    std::unordered_map<std::string, ConnectionPool> pools_;
    mutable std::mutex mutex_;
    std::atomic<bool> running_{false};
    std::thread health_check_thread_;
    StreamConfig config_;
    
private:
    struct PoolEntry {
        std::queue<std::shared_ptr<ManagedSocket>> available;
        std::unordered_map<int, std::shared_ptr<ManagedSocket>> in_use;
        size_t total_connections = 0;
    };
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, PoolEntry> connection_pools_;
    StreamConfig config_;
    std::atomic<bool> running_{false};
    std::thread health_check_thread_;
};

} // namespace network
} // namespace streaming
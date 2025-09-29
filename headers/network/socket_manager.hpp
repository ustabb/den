// headers/network/socket_manager.hpp
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <queue>
#include <cstdint>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <BaseTsd.h>
    typedef SSIZE_T ssize_t;
    #define MSG_DONTWAIT 0
    #define MSG_NOSIGNAL 0
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
#endif

namespace streaming {
namespace network {

class ManagedSocket {
private:
    int sockfd_;
    std::string host_;
    uint16_t port_;
    time_t last_used_;
    bool is_healthy_;
    sockaddr_in remote_addr_;
    bool connected_;

public:
    ManagedSocket(const std::string& host, uint16_t port);
    ManagedSocket(ManagedSocket&& other) noexcept;
    ManagedSocket& operator=(ManagedSocket&& other) noexcept;
    ManagedSocket(const ManagedSocket&) = delete;
    ManagedSocket& operator=(const ManagedSocket&) = delete;
    ~ManagedSocket();

    int get_socket() const;
    void mark_used();
    const std::string& get_host() const;
    uint16_t get_port() const;
    bool check_health();
    ssize_t send_data(const std::vector<uint8_t>& data) noexcept;
    ssize_t send(const uint8_t* data, size_t size) noexcept;
    bool is_connected() const;
    void close();
};

struct ConnectionPool {
    std::queue<std::shared_ptr<ManagedSocket>> available;
    size_t total_connections = 0;
    size_t max_size = 16;
};

struct StreamConfig {
    // Config fields - örnek olarak
    uint32_t health_check_interval = 10;
    uint32_t connection_timeout = 5;
};

class SocketManager {
public:
    SocketManager();
    ~SocketManager();

    bool initialize(const StreamConfig& config);
    bool connect(const std::string& host, uint16_t port);
    ssize_t send(const std::string& host, uint16_t port, const std::vector<uint8_t>& data);
    std::shared_ptr<ManagedSocket> acquire_connection(const std::string& host, uint16_t port);
    bool stream_video_data(const std::string& host, uint16_t port, const uint8_t* data, size_t size);

    static SocketManager& get_instance();

private:
    void cleanup();
    void health_check();
    void health_check_loop();
    std::shared_ptr<ManagedSocket> get_connection(const std::string& host, uint16_t port);
    void return_connection(const std::string& host, uint16_t port, std::shared_ptr<ManagedSocket> socket);

    StreamConfig config_;
    std::atomic<bool> running_;
    std::thread health_check_thread_;
    std::mutex mutex_;
    std::unordered_map<std::string, ConnectionPool> connection_pools_;
};

} // namespace network
} // namespace streaming
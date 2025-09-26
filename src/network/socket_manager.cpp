// src/network/socket_manager.cpp
#include "streaming/network/socket_manager.hpp"
#include "streaming/utils/logger.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

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
    
public:
    ManagedSocket(const std::string& host, uint16_t port) 
        : host_(host), port_(port), last_used_(time(nullptr)), is_healthy_(true) {
        
        // Socket creation
        sockfd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (sockfd_ < 0) {
            throw std::runtime_error("Socket creation failed: " + std::string(strerror(errno)));
        }
        
        // TCP optimizations from previous conversation
        if (!TCPOptimizer::optimize_socket(sockfd_)) {
            close(sockfd_);
            throw std::runtime_error("TCP optimization failed");
        }
        
        // Setup remote address
        remote_addr_.sin_family = AF_INET;
        remote_addr_.sin_port = htons(port);
        if (inet_pton(AF_INET, host.c_str(), &remote_addr_.sin_addr) <= 0) {
            close(sockfd_);
            throw std::runtime_error("Invalid address: " + host);
        }
        
        // Non-blocking connect
        int result = connect(sockfd_, (sockaddr*)&remote_addr_, sizeof(remote_addr_));
        if (result < 0 && errno != EINPROGRESS) {
            close(sockfd_);
            throw std::runtime_error("Connection failed: " + std::string(strerror(errno)));
        }
        
        LOG_INFO("Socket created for {}:{}", host, port);
    }
    
    ~ManagedSocket() {
        if (sockfd_ >= 0) {
            close(sockfd_);
            LOG_DEBUG("Socket closed for {}:{}", host_, port_);
        }
    }
    
    // Move semantics - important for connection pooling
    ManagedSocket(ManagedSocket&& other) noexcept 
        : sockfd_(other.sockfd_), host_(std::move(other.host_)), 
          port_(other.port_), last_used_(other.last_used_),
          is_healthy_(other.is_healthy_), remote_addr_(other.remote_addr_) {
        other.sockfd_ = -1; // Prevent double close
    }
    
    ManagedSocket& operator=(ManagedSocket&& other) noexcept {
        if (this != &other) {
            if (sockfd_ >= 0) close(sockfd_);
            sockfd_ = other.sockfd_;
            host_ = std::move(other.host_);
            port_ = other.port_;
            last_used_ = other.last_used_;
            is_healthy_ = other.is_healthy_;
            remote_addr_ = other.remote_addr_;
            other.sockfd_ = -1;
        }
        return *this;
    }
    
    // Delete copy operations
    ManagedSocket(const ManagedSocket&) = delete;
    ManagedSocket& operator=(const ManagedSocket&) = delete;
    
    int get_socket() const { return sockfd_; }
    void mark_used() { last_used_ = time(nullptr); }
    const std::string& get_host() const { return host_; }
    uint16_t get_port() const { return port_; }
    
    bool check_health() {
        if (sockfd_ < 0) {
            is_healthy_ = false;
            return false;
        }
        
        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(sockfd_, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
            is_healthy_ = false;
        } else {
            is_healthy_ = (error == 0);
        }
        
        return is_healthy_;
    }
    
    ssize_t send_data(const std::vector<uint8_t>& data) {
        if (!is_healthy_ && !check_health()) {
            return -1;
        }
        
        ssize_t result = send(sockfd_, data.data(), data.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
        if (result < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                is_healthy_ = false;
            }
        } else {
            mark_used();
        }
        
        return result;
    }
};


SocketManager::SocketManager() {
    LOG_DEBUG("SocketManager initialized");
}

SocketManager::~SocketManager() {
    cleanup();
    LOG_DEBUG("SocketManager destroyed");
}

bool SocketManager::initialize(const StreamConfig& config) {
    config_ = config;
    running_.store(true, std::memory_order_release);
    
    // Start health check thread
    health_check_thread_ = std::thread([this]() {
        while (running_.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            health_check();
        }
    });
    
    LOG_INFO("SocketManager initialized with config");
    return true;
}

bool SocketManager::connect(const std::string& host, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = host + ":" + std::to_string(port);
    
    auto& pool = connection_pools_[key];
    
    try {
        auto socket = std::make_shared<ManagedSocket>(host, port);
        pool.available.push(socket);
        pool.total_connections++;
        
        LOG_INFO("Connected to {}:{} - Pool size: {}", host, port, pool.total_connections);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Connection to {}:{} failed: {}", host, port, e.what());
        return false;
    }
}

ssize_t SocketManager::send(const std::string& host, uint16_t port, 
                           const std::vector<uint8_t>& data) {
    auto socket = get_connection(host, port);
    if (!socket) {
        LOG_ERROR("No available socket for {}:{}", host, port);
        return -1;
    }
    
    ssize_t result = socket->send_data(data);
    return_connection(host, port, socket);
    
    return result;
}

} // namespace network
} // namespace streaming

// Diğer fonksiyonlar için devam edelim...

// src/network/socket_manager.cpp
#include "streaming/network/socket_manager.hpp"
#include "streaming/utils/logger.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

namespace streaming {
namespace network {

// ManagedSocket implementation
ManagedSocket::ManagedSocket(const std::string& host, uint16_t port) 
    : host_(host), port_(port) {
    
    sockfd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sockfd_ < 0) {
        LOG_ERROR("Socket creation failed for {}:{} - {}", host, port, strerror(errno));
        return;
    }
    
    // Apply TCP optimizations from previous work
    if (!TCPOptimizer::optimize_socket(sockfd_)) {
        close();
        LOG_ERROR("TCP optimization failed for {}:{}", host, port);
        return;
    }
    
    // Setup remote address
    remote_addr_.sin_family = AF_INET;
    remote_addr_.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &remote_addr_.sin_addr) <= 0) {
        close();
        LOG_ERROR("Invalid address: {}", host);
        return;
    }
    
    // Non-blocking connect
    int result = connect(sockfd_, (sockaddr*)&remote_addr_, sizeof(remote_addr_));
    if (result < 0 && errno != EINPROGRESS) {
        close();
        LOG_ERROR("Connection failed to {}:{} - {}", host, port, strerror(errno));
        return;
    }
    
    // Wait for connection with timeout
    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(sockfd_, &write_fds);
    
    timeval timeout{5, 0}; // 5 second timeout
    
    if (select(sockfd_ + 1, nullptr, &write_fds, nullptr, &timeout) > 0) {
        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(sockfd_, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
            close();
            LOG_ERROR("Connection failed to {}:{} - error: {}", host, port, error);
            return;
        }
        connected_ = true;
        LOG_INFO("Connected to {}:{}", host, port);
    } else {
        close();
        LOG_ERROR("Connection timeout to {}:{}", host, port);
    }
}

ssize_t ManagedSocket::send(const uint8_t* data, size_t size) {
    if (!connected_) return -1;
    
    ssize_t sent = ::send(sockfd_, data, size, MSG_DONTWAIT | MSG_NOSIGNAL);
    if (sent < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            connected_ = false;
            LOG_ERROR("Send failed to {}:{} - {}", host_, port_, strerror(errno));
        }
    }
    return sent;
}

void ManagedSocket::close() {
    if (sockfd_ >= 0) {
        ::close(sockfd_);
        sockfd_ = -1;
        connected_ = false;
    }
}

// SocketManager implementation
SocketManager& SocketManager::get_instance() {
    static SocketManager instance;
    return instance;
}

bool SocketManager::initialize(const StreamConfig& config) {
    config_ = config;
    running_.store(true, std::memory_order_release);
    
    health_check_thread_ = std::thread([this]() {
        health_check_loop();
    });
    
    LOG_INFO("SocketManager initialized");
    return true;
}

std::shared_ptr<ManagedSocket> SocketManager::acquire_connection(const std::string& host, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = host + ":" + std::to_string(port);
    
    auto& pool = pools_[key];
    
    // Try to get from available pool
    while (!pool.available.empty()) {
        auto socket = pool.available.front();
        pool.available.pop();
        
        if (socket->is_connected()) {
            return socket;
        }
    }
    
    // Create new connection if under limit
    if (pool.total_created < pool.max_size) {
        try {
            auto socket = std::make_shared<ManagedSocket>(host, port);
            if (socket->is_connected()) {
                pool.total_created++;
                return socket;
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to create socket for {}:{} - {}", host, port, e.what());
        }
    }
    
    return nullptr;
}

bool SocketManager::stream_video_data(const std::string& host, uint16_t port, 
                                     const uint8_t* data, size_t size) {
    auto socket = acquire_connection(host, port);
    if (!socket) {
        LOG_ERROR("No available connection to {}:{}", host, port);
        return false;
    }
    
    ssize_t sent = socket->send(data, size);
    release_connection(host, port, socket);
    
    if (sent != static_cast<ssize_t>(size)) {
        LOG_WARN("Partial send to {}:{} - {}/{} bytes", host, port, sent, size);
        return false;
    }
    
    return true;
}
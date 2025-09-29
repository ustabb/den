// source/network/socket_manager.cpp
#include "network/socket_manager.hpp"
#include <spdlog/spdlog.h>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
#endif

using namespace streaming::network;

// Platform-specific send flags
#ifdef _WIN32
    static constexpr int SEND_FLAGS = 0;
#else
    static constexpr int SEND_FLAGS = MSG_DONTWAIT | MSG_NOSIGNAL;
#endif

// ManagedSocket implementation
ManagedSocket::ManagedSocket(const std::string& host, uint16_t port)
    : sockfd_(-1), host_(host), port_(port), last_used_(time(nullptr)), 
      is_healthy_(false), connected_(false) {
    
#ifdef _WIN32
    // Windows socket initialization
    sockfd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd_ == INVALID_SOCKET) {
        throw std::runtime_error("socket() failed");
    }
    
    // Set non-blocking mode
    u_long mode = 1;
    if (ioctlsocket(sockfd_, FIONBIO, &mode) != 0) {
        closesocket(sockfd_);
        throw std::runtime_error("ioctlsocket() failed");
    }
#else
    // Linux socket initialization
    sockfd_ = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sockfd_ < 0) {
        throw std::runtime_error("socket() failed");
    }
#endif

    // Prepare remote address
    std::memset(&remote_addr_, 0, sizeof(remote_addr_));
    remote_addr_.sin_family = AF_INET;
    remote_addr_.sin_port = htons(port_);
    
    if (inet_pton(AF_INET, host.c_str(), &remote_addr_.sin_addr) <= 0) {
        close();
        throw std::runtime_error("Invalid IP address");
    }

    // Non-blocking connect
    int result = ::connect(sockfd_, reinterpret_cast<sockaddr*>(&remote_addr_), sizeof(remote_addr_));
    
#ifdef _WIN32
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK || error == WSAEINPROGRESS) {
            connected_ = true;
            is_healthy_ = true;
        } else {
            close();
            throw std::runtime_error("connect() failed");
        }
    } else {
        connected_ = true;
        is_healthy_ = true;
    }
#else
    if (result < 0) {
        if (errno == EINPROGRESS) {
            connected_ = true;
            is_healthy_ = true;
        } else {
            close();
            throw std::runtime_error("connect() failed");
        }
    } else {
        connected_ = true;
        is_healthy_ = true;
    }
#endif
}

ManagedSocket::~ManagedSocket() { 
    close(); 
}

// Move constructor
ManagedSocket::ManagedSocket(ManagedSocket&& other) noexcept
    : sockfd_(other.sockfd_), host_(std::move(other.host_)), port_(other.port_),
      last_used_(other.last_used_), is_healthy_(other.is_healthy_),
      remote_addr_(other.remote_addr_), connected_(other.connected_) {
    other.sockfd_ = -1;
    other.connected_ = false;
}

// Move assignment operator
ManagedSocket& ManagedSocket::operator=(ManagedSocket&& other) noexcept {
    if (this != &other) {
        close();
        sockfd_ = other.sockfd_;
        host_ = std::move(other.host_);
        port_ = other.port_;
        last_used_ = other.last_used_;
        is_healthy_ = other.is_healthy_;
        remote_addr_ = other.remote_addr_;
        connected_ = other.connected_;
        
        other.sockfd_ = -1;
        other.connected_ = false;
    }
    return *this;
}

int ManagedSocket::get_socket() const { 
    return sockfd_; 
}

void ManagedSocket::mark_used() { 
    last_used_ = time(nullptr); 
}

const std::string& ManagedSocket::get_host() const { 
    return host_; 
}

uint16_t ManagedSocket::get_port() const { 
    return port_; 
}

bool ManagedSocket::is_connected() const { 
    return connected_; 
}

bool ManagedSocket::check_health() {
    if (sockfd_ < 0) {
        is_healthy_ = false;
        return false;
    }
    
    int error = 0;
    socklen_t len = sizeof(error);
    
#ifdef _WIN32
    if (getsockopt(sockfd_, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &len) == SOCKET_ERROR) {
        is_healthy_ = false;
    } else {
        is_healthy_ = (error == 0);
    }
#else
    if (getsockopt(sockfd_, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
        is_healthy_ = false;
    } else {
        is_healthy_ = (error == 0);
    }
#endif
    
    return is_healthy_;
}

ssize_t ManagedSocket::send_data(const std::vector<uint8_t>& data) noexcept {
    return send(data.data(), data.size());
}

ssize_t ManagedSocket::send(const uint8_t* data, size_t size) noexcept {
    if (!connected_ || sockfd_ < 0) {
        return -1;
    }
    
#ifdef _WIN32
    int result = ::send(sockfd_, reinterpret_cast<const char*>(data), static_cast<int>(size), SEND_FLAGS);
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK) {
            connected_ = false;
        }
        return -1;
    }
#else
    ssize_t result = ::send(sockfd_, data, size, SEND_FLAGS);
    if (result < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            connected_ = false;
        }
        return -1;
    }
#endif

    mark_used();
    return result;
}

void ManagedSocket::close() {
    if (sockfd_ >= 0) {
#ifdef _WIN32
        closesocket(sockfd_);
#else
        ::close(sockfd_);
#endif
        sockfd_ = -1;
        connected_ = false;
    }
}

// SocketManager implementation
SocketManager::SocketManager() : running_(false) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

SocketManager::~SocketManager() {
    running_.store(false);
    if (health_check_thread_.joinable()) {
        health_check_thread_.join();
    }
    cleanup();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool SocketManager::initialize(const StreamConfig& config) {
    config_ = config;
    running_.store(true);
    health_check_thread_ = std::thread([this]() { health_check_loop(); });
    return true;
}

void SocketManager::health_check_loop() {
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(config_.health_check_interval));
        health_check();
    }
}

bool SocketManager::connect(const std::string& host, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = host + ":" + std::to_string(port);
    auto& pool = connection_pools_[key];
    
    try {
        auto socket = std::make_shared<ManagedSocket>(host, port);
        if (pool.available.size() < pool.max_size) {
            pool.available.push(socket);
            pool.total_connections++;
            spdlog::info("Connected to {}:{} - Pool size: {}", host, port, pool.total_connections);
            return true;
        }
    } catch (const std::exception& e) {
        spdlog::error("Connection to {}:{} failed: {}", host, port, e.what());
    }
    
    return false;
}

std::shared_ptr<ManagedSocket> SocketManager::get_connection(const std::string& host, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = host + ":" + std::to_string(port);
    auto it = connection_pools_.find(key);
    
    if (it == connection_pools_.end()) {
        return nullptr;
    }
    
    auto& pool = it->second;
    if (!pool.available.empty()) {
        auto socket = pool.available.front();
        pool.available.pop();
        return socket;
    }
    
    return nullptr;
}

void SocketManager::return_connection(const std::string& host, uint16_t port, std::shared_ptr<ManagedSocket> socket) {
    if (!socket) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = host + ":" + std::to_string(port);
    auto& pool = connection_pools_[key];
    
    if (socket->is_connected() && pool.available.size() < pool.max_size) {
        pool.available.push(socket);
    } else {
        pool.total_connections--;
    }
}

std::shared_ptr<ManagedSocket> SocketManager::acquire_connection(const std::string& host, uint16_t port) {
    return get_connection(host, port);
}

bool SocketManager::stream_video_data(const std::string& host, uint16_t port, const uint8_t* data, size_t size) {
    auto socket = get_connection(host, port);
    if (!socket) {
        return false;
    }
    
    ssize_t sent = socket->send(data, size);
    return_connection(host, port, socket);
    return sent >= 0;
}

ssize_t SocketManager::send(const std::string& host, uint16_t port, const std::vector<uint8_t>& data) {
    auto socket = get_connection(host, port);
    if (!socket) {
        return -1;
    }
    
    ssize_t result = socket->send_data(data);
    return_connection(host, port, socket);
    return result;
}

void SocketManager::health_check() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto it = connection_pools_.begin(); it != connection_pools_.end(); ) {
        auto& pool = it->second;
        
        // Remove unhealthy sockets
        size_t initial_size = pool.available.size();
        std::queue<std::shared_ptr<ManagedSocket>> healthy_queue;
        
        while (!pool.available.empty()) {
            auto socket = pool.available.front();
            pool.available.pop();
            
            if (socket && socket->check_health()) {
                healthy_queue.push(socket);
            } else {
                pool.total_connections--;
            }
        }
        
        pool.available = std::move(healthy_queue);
        
        // Remove empty pools
        if (pool.total_connections == 0) {
            it = connection_pools_.erase(it);
        } else {
            ++it;
        }
    }
}

void SocketManager::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    connection_pools_.clear();
}

SocketManager& SocketManager::get_instance() {
    static SocketManager instance;
    return instance;
}
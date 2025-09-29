#include "network/udp_server.hpp"
#include <spdlog/spdlog.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <cstring>
#endif

using namespace network;

BasicUDPServer::BasicUDPServer() : sockfd_(-1), running_(false) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

BasicUDPServer::~BasicUDPServer() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool BasicUDPServer::start(uint16_t port) {
#ifdef _WIN32
    sockfd_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd_ == INVALID_SOCKET) {
        spdlog::error("Failed to create UDP socket");
        return false;
    }
#else
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        spdlog::error("Failed to create UDP socket");
        return false;
    }
#endif

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd_, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        spdlog::error("Failed to bind UDP socket to port {}", port);
        stop();
        return false;
    }

    running_ = true;
    spdlog::info("UDP server started on port {}", port);
    return true;
}

void BasicUDPServer::stop() {
    if (sockfd_ >= 0) {
#ifdef _WIN32
        closesocket(sockfd_);
#else
        close(sockfd_);
#endif
        sockfd_ = -1;
    }
    running_ = false;
    spdlog::info("UDP server stopped");
}

bool BasicUDPServer::send(const std::string& host, uint16_t port, const std::vector<uint8_t>& data) {
    if (!running_ || sockfd_ < 0) {
        return false;
    }

    sockaddr_in dest_addr{};
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &dest_addr.sin_addr);

#ifdef _WIN32
    int sent = sendto(sockfd_, (const char*)data.data(), (int)data.size(), 0,
                     (sockaddr*)&dest_addr, sizeof(dest_addr));
    if (sent == SOCKET_ERROR) {
        spdlog::error("Failed to send UDP data");
        return false;
    }
#else
    ssize_t sent = sendto(sockfd_, data.data(), data.size(), 0,
                         (sockaddr*)&dest_addr, sizeof(dest_addr));
    if (sent < 0) {
        spdlog::error("Failed to send UDP data");
        return false;
    }
#endif

    return true;
}

std::vector<uint8_t> BasicUDPServer::receive() {
    std::vector<uint8_t> buffer(4096);
    
    if (!running_ || sockfd_ < 0) {
        return {};
    }

#ifdef _WIN32
    int received = recv(sockfd_, (char*)buffer.data(), (int)buffer.size(), 0);
    if (received == SOCKET_ERROR) {
        return {};
    }
    buffer.resize(received);
#else
    ssize_t received = recv(sockfd_, buffer.data(), buffer.size(), 0);
    if (received < 0) {
        return {};
    }
    buffer.resize(received);
#endif

    return buffer;
}

bool BasicUDPServer::is_running() const {
    return running_;
}
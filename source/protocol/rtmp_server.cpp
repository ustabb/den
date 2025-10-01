#include "rtmp_server.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

RtmpServer::RtmpServer(int port) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);
    running = true;
}

void RtmpServer::start() {
    while (running) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd >= 0) {
            std::thread(&RtmpServer::handleClient, this, client_fd).detach();
        }
    }
}

void RtmpServer::stop() {
    running = false;
    close(server_fd);
}

void RtmpServer::handleClient(int client_fd) {
    std::cout << "RTMP client connected" << std::endl;
    // TODO: implement RTMP handshake & chunk stream parsing
    close(client_fd);
}

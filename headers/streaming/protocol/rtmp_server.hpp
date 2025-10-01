#pragma once
#include <string>
#include <thread>
#include <unordered_map>

class RtmpServer {
public:
    RtmpServer(int port);
    void start();
    void stop();

private:
    int server_fd;
    bool running;
    void handleClient(int client_fd);
};

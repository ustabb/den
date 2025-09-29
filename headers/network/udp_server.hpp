#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <memory>

namespace network {

class UDPServer {
public:
    virtual ~UDPServer() = default;
    
    virtual bool start(uint16_t port) = 0;
    virtual void stop() = 0;
    virtual bool send(const std::string& host, uint16_t port, const std::vector<uint8_t>& data) = 0;
    virtual std::vector<uint8_t> receive() = 0;
    virtual bool is_running() const = 0;
};

class BasicUDPServer : public UDPServer {
private:
    int sockfd_;
    bool running_;
    
public:
    BasicUDPServer();
    ~BasicUDPServer();
    
    bool start(uint16_t port) override;
    void stop() override;
    bool send(const std::string& host, uint16_t port, const std::vector<uint8_t>& data) override;
    std::vector<uint8_t> receive() override;
    bool is_running() const override;
};

} // namespace network
// source/network/http_session.cpp
#include "network/http_session.hpp"
#include <spdlog/spdlog.h>

using namespace network;

HttpSession::HttpSession(std::shared_ptr<tcp::socket> sock)
: sock_(std::move(sock)), buffer_(8192) {}

void HttpSession::start() {
    spdlog::info("HttpSession: started with {}", sock_->remote_endpoint().address().to_string());
    do_read();
}

void HttpSession::do_read() {
    auto self = shared_from_this();
    sock_->async_read_some(
        boost::asio::buffer(buffer_),
        [self](boost::system::error_code ec, std::size_t bytes) {
            self->on_read(ec, bytes);
        });
}

void HttpSession::on_read(boost::system::error_code ec, std::size_t bytes) {
    if (ec) {
        spdlog::warn("HttpSession: read error: {}", ec.message());
        return;
    }
    std::string request(buffer_.begin(), buffer_.begin() + bytes);
    spdlog::debug("HttpSession: received:\n{}", request);

    // Basit cevap
    std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello World";
    boost::asio::async_write(*sock_, boost::asio::buffer(response),
        [self=shared_from_this()](boost::system::error_code ec, std::size_t) {
            if (ec) {
                spdlog::warn("HttpSession: write error: {}", ec.message());
            }
        });
}

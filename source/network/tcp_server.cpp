// source/network/tcp_server.cpp
#include "network/tcp_server.hpp"
#include "network/protocol.hpp"
#include <spdlog/spdlog.h>

using namespace network;

TcpServer::TcpServer(boost::asio::io_context& ioc, unsigned short port, std::size_t peek_bytes)
: ioc_(ioc),
  acceptor_(ioc, tcp::endpoint(tcp::v4(), port)),
  peek_bytes_(peek_bytes) {
    // set acceptor options
    boost::system::error_code ec;
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec) {
        spdlog::warn("TcpServer: set_option reuse_address failed: {}", ec.message());
    }
}

TcpServer::~TcpServer() {
    stop();
}

void TcpServer::run() {
    do_accept();
}

void TcpServer::stop() {
    boost::system::error_code ec;
    acceptor_.close(ec);
    if (ec) spdlog::warn("TcpServer stop: {}", ec.message());
}

void TcpServer::set_error_callback(std::function<void(const boost::system::error_code&)> cb) {
    error_cb_ = std::move(cb);
}

void TcpServer::do_accept() {
    auto self = shared_from_this();
    acceptor_.async_accept([this, self](boost::system::error_code ec, tcp::socket socket) {
        on_accept(ec, std::move(socket));
    });
}

void TcpServer::on_accept(boost::system::error_code ec, tcp::socket socket) {
    if (ec) {
        spdlog::error("TcpServer accept error: {}", ec.message());
        if (error_cb_) error_cb_(ec);

        // küçük gecikmeyle yeniden dene
        auto timer = std::make_shared<boost::asio::steady_timer>(ioc_, std::chrono::milliseconds(50));
        timer->async_wait([this, self = shared_from_this(), timer](const boost::system::error_code&){
            do_accept();
        });
        return;
    }

    spdlog::info("TcpServer: incoming connection from {}", socket.remote_endpoint().address().to_string());

    auto sock = std::make_shared<tcp::socket>(std::move(socket));
    auto buf  = std::make_shared<std::vector<uint8_t>>(peek_bytes_);

    auto self = shared_from_this();
    sock->async_receive(
        boost::asio::buffer(*buf),
        boost::asio::socket_base::message_peek,
        [self, sock, buf](boost::system::error_code ec, std::size_t bytes_peeked) {
            if (ec) {
                spdlog::warn("TcpServer: peek error: {}", ec.message());
                ProtocolRouter::route(sock, {}); // fallback
            } else {
                buf->resize(bytes_peeked);
                spdlog::debug("TcpServer: peeked {} bytes", bytes_peeked);
                ProtocolRouter::route(sock, *buf);
            }
        }
    );

    // yeni bağlantılar için accept döngüsünü devam ettir
    do_accept();
}
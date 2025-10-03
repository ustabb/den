#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <functional>
#include <string>

namespace network {

using tcp = boost::asio::ip::tcp;

class ProtocolRouter; // forward
class TcpServer : public std::enable_shared_from_this<TcpServer> {
public:
    TcpServer(boost::asio::io_context& ioc, unsigned short port, std::size_t peek_bytes = 8);
    ~TcpServer();

    void run();

    void stop();

    void set_error_callback(std::function<void(const boost::system::error_code&)> cb);

private:
    void do_accept();
    void on_accept(boost::system::error_code ec, tcp::socket socket);

private:
    boost::asio::io_context& ioc_;
    tcp::acceptor acceptor_;
    std::size_t peek_bytes_;
    std::function<void(const boost::system::error_code&)> error_cb_;
};

} // namespace network

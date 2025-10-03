// headers/network/http_session.hpp
#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <vector>

namespace network {

using tcp = boost::asio::ip::tcp;

class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    explicit HttpSession(std::shared_ptr<tcp::socket> sock);

    void start();

private:
    void do_read();
    void on_read(boost::system::error_code ec, std::size_t bytes);

    std::shared_ptr<tcp::socket> sock_;
    std::vector<uint8_t> buffer_;
};

} // namespace network

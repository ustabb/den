// src/server/streaming_server.cpp
#include "streaming/server/streaming_server.hpp"
#include "streaming/server/session_handler.hpp"
#include "streaming/server/stream_manager.hpp"
#include "streaming/server/http_flv_handler.hpp"
#include "streaming/server/hls_handler.hpp"
#include "streaming/server/rtmp_handler.hpp"
#include "streaming/utils/logger.hpp"
#include <boost/asio.hpp>
#include <thread>

namespace streaming {
namespace server {

StreamingServer::StreamingServer() {
    session_handler_ = std::make_unique<SessionHandler>();
    stream_manager_ = std::make_unique<StreamManager>();
    http_flv_handler_ = std::make_unique<HttpFlvHandler>();
    hls_handler_ = std::make_unique<HlsHandler>();
    rtmp_handler_ = std::make_unique<RtmpHandler>();
}

bool StreamingServer::initialize(const ServerConfig& config) {
    config_ = config;
    
    try {
        // Initialize HTTP acceptor
        http_acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>(
            io_context_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), config.http_port));
        
        // Initialize RTMP acceptor
        rtmp_acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>(
            io_context_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), config.rtmp_port));
        
        // Initialize WebSocket acceptor
        websocket_acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>(
            io_context_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), config.websocket_port));
        
        LOG_INFO("StreamingServer initialized on ports: HTTP={}, RTMP={}, WebSocket={}", 
                 config.http_port, config.rtmp_port, config.websocket_port);
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize StreamingServer: {}", e.what());
        return false;
    }
}

void StreamingServer::start() {
    if (running_.exchange(true, std::memory_order_acq_rel)) {
        LOG_WARN("StreamingServer already running");
        return;
    }
    
    // Start acceptors
    start_http_server();
    start_rtmp_server();
    start_websocket_server();
    
    // Start worker threads
    for (uint32_t i = 0; i < config_.worker_threads; ++i) {
        worker_threads_.emplace_back([this]() {
            io_context_.run();
        });
    }
    
    // Start cleanup thread
    cleanup_thread_ = std::thread([this]() {
        while (running_.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::seconds(30));
            cleanup_expired_sessions();
        }
    });
    
    LOG_INFO("StreamingServer started with {} worker threads", config_.worker_threads);
}

void StreamingServer::start_http_server() {
    auto accept_http = [this]() {
        http_acceptor_->async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                std::thread(&StreamingServer::handle_http_request, this, std::move(socket)).detach();
            }
            start_http_server(); // Continue accepting
        });
    };
    
    accept_http();
    LOG_INFO("HTTP server listening on port {}", config_.http_port);
}

void StreamingServer::handle_http_request(boost::asio::ip::tcp::socket socket) {
    try {
        boost::beast::flat_buffer buffer;
        boost::beast::http::request<boost::beast::http::string_body> request;
        boost::beast::http::read(socket, buffer, request);
        
        boost::beast::http::response<boost::beast::http::dynamic_body> response;
        
        // Route request to appropriate handler
        std::string path = request.target();
        
        if (path.find(".flv") != std::string::npos && config_.enable_http_flv) {
            http_flv_handler_->handle_request(request, response);
        } else if (path.find(".m3u8") != std::string::npos && config_.enable_hls) {
            hls_handler_->handle_request(request, response);
        } else {
            // Serve static files
            response.result(boost::beast::http::status::not_found);
            response.set(boost::beast::http::field::content_type, "text/html");
            boost::beast::ostream(response.body()) << "404 Not Found";
        }
        
        boost::beast::http::write(socket, response);
        socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
        
    } catch (const std::exception& e) {
        LOG_ERROR("HTTP request handling error: {}", e.what());
    }
}

void StreamingServer::start_rtmp_server() {
    auto accept_rtmp = [this]() {
        rtmp_acceptor_->async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                std::thread(&RtmpHandler::handle_connection, rtmp_handler_.get(), std::move(socket)).detach();
            }
            start_rtmp_server(); // Continue accepting
        });
    };
    
    accept_rtmp();
    LOG_INFO("RTMP server listening on port {}", config_.rtmp_port);
}

bool StreamingServer::push_stream_data(const std::string& stream_name, const uint8_t* data, 
                                     size_t size, uint64_t timestamp) {
    if (!stream_manager_->stream_exists(stream_name)) {
        if (!create_stream(stream_name)) {
            LOG_ERROR("Failed to create stream: {}", stream_name);
            return false;
        }
    }
    
    // Assume it's video data for simplicity
    // In real implementation, you'd detect the data type
    stream_manager_->push_stream_data(stream_name, data, size, timestamp, true, false);
    
    return true;
}

void StreamingServer::stop() {
    if (!running_.exchange(false, std::memory_order_acq_rel)) {
        return;
    }
    
    io_context_.stop();
    
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
    
    LOG_INFO("StreamingServer stopped");
}

} // namespace server
} // namespace streaming
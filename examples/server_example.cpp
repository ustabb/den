// examples/server_example.cpp
#include "streaming/server/streaming_server.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace streaming;

class ServerTester {
private:
    server::StreamingServer server_;
    
public:
    bool start_server() {
        server::StreamingServer::ServerConfig config;
        config.http_port = 8080;
        config.rtmp_port = 1935;
        config.websocket_port = 8081;
        config.worker_threads = 4;
        config.document_root = "./www";
        config.enable_hls = true;
        config.enable_http_flv = true;
        config.enable_rtmp = true;
        
        if (!server_.initialize(config)) {
            std::cerr << "Failed to initialize server" << std::endl;
            return false;
        }
        
        server_.start();
        std::cout << "🚀 Streaming Server started successfully!" << std::endl;
        std::cout << "   HTTP-FLV: http://localhost:8080/stream.flv" << std::endl;
        std::cout << "   HLS: http://localhost:8080/stream.m3u8" << std::endl;
        std::cout << "   RTMP: rtmp://localhost:1935/live/stream" << std::endl;
        
        return true;
    }
    
    void simulate_stream_publishing() {
        std::cout << "\n🎥 Simulating stream publishing..." << std::endl;
        
        // Create a test stream
        server_.create_stream("test_stream");
        
        // Simulate publishing video data
        for (int i = 0; i < 100; i++) {
            std::vector<uint8_t> video_data(1024, static_cast<uint8_t>(i % 256));
            uint64_t timestamp = i * 33 * 1000; // 33ms per frame
            
            server_.push_stream_data("test_stream", video_data.data(), video_data.size(), timestamp);
            
            if (i % 30 == 0) {
                std::cout << "Published frame " << i << " to stream 'test_stream'" << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(33)); // 30fps
        }
    }
    
    void print_statistics() {
        auto stats = server_.get_statistics();
        
        std::cout << "\n📊 Server Statistics:" << std::endl;
        std::cout << "====================" << std::endl;
        std::cout << "Active Connections: " << stats.active_connections << std::endl;
        std::cout << "Total Streams: " << stats.total_streams << std::endl;
        std::cout << "Bytes Sent: " << stats.bytes_sent << std::endl;
        std::cout << "Bytes Received: " << stats.bytes_received << std::endl;
        std::cout << "Active Sessions: " << stats.active_sessions << std::endl;
    }
};

int main() {
    try {
        ServerTester tester;
        
        if (!tester.start_server()) {
            std::cerr << "Failed to start server" << std::endl;
            return 1;
        }
        
        // Let server start up
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // Test stream publishing
        tester.simulate_stream_publishing();
        
        // Print statistics
        tester.print_statistics();
        
        std::cout << "\n✅ Server test completed successfully!" << std::endl;
        std::cout << "Press Enter to stop the server..." << std::endl;
        std::cin.get();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
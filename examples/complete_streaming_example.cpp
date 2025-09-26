// examples/complete_streaming_example.cpp
#include "streaming/engine/streaming_pipeline.hpp"
#include <iostream>
#include <csignal>

using namespace streaming;

std::atomic<bool> running{true};

void signal_handler(int signal) {
    std::cout << "\n🛑 Received signal " << signal << ", shutting down...\n";
    running.store(false);
}

int main() {
    // Signal handling
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    try {
        // Streaming configuration
        StreamConfig config;
        config.host = "192.168.1.100";  // Hedef IP
        config.port = 8080;             // Hedef port
        config.low_latency_mode = true;
        config.zero_copy_enabled = true;
        config.max_packet_size = 1400;
        
        // Pipeline oluştur ve başlat
        StreamingPipeline pipeline;
        
        pipeline.set_status_callback([](const std::string& status) {
            std::cout << "📡 Status: " << status << std::endl;
        });
        
        pipeline.set_error_callback([](const std::string& error) {
            std::cerr << "❌ Error: " << error << std::endl;
        });
        
        std::cout << "🚀 Initializing streaming pipeline..." << std::endl;
        
        if (!pipeline.initialize(config)) {
            std::cerr << "Failed to initialize pipeline!" << std::endl;
            return -1;
        }
        
        std::cout << "✅ Pipeline initialized. Starting stream..." << std::endl;
        pipeline.start_streaming();
        
        // Ana döngü
        std::cout << "🎥 Streaming to " << config.host << ":" << config.port << std::endl;
        std::cout << "Press Ctrl+C to stop..." << std::endl;
        
        while (running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        std::cout << "🛑 Stopping pipeline..." << std::endl;
        pipeline.stop_streaming();
        
        std::cout << "✅ Streaming completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "💥 Critical error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
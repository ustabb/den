// examples/protocol_test.cpp
#include "streaming/protocol/streaming_protocol.hpp"
#include <iostream>
#include <chrono>
#include <thread>

using namespace streaming;

class ProtocolTester {
private:
    protocol::StreamingProtocol protocol_;
    
public:
    bool initialize() {
        protocol::StreamingProtocol::ProtocolConfig config;
        config.session_id = 123456;
        config.initial_bitrate = 2000000; // 2 Mbps
        config.max_bitrate = 5000000;     // 5 Mbps
        config.min_bitrate = 500000;      // 500 Kbps
        config.enable_fec = true;
        config.enable_retransmission = true;
        config.max_latency_ms = 50;       // Ultra-low latency
        
        if (!protocol_.initialize(config)) {
            std::cerr << "Failed to initialize protocol" << std::endl;
            return false;
        }
        
        return true;
    }
    
    void test_video_streaming() {
        std::cout << "🎬 Testing video streaming..." << std::endl;
        
        // Simulate video frames
        for (int frame = 0; frame < 100; ++frame) {
            std::vector<uint8_t> frame_data(1024 * 1024, 0xAA); // 1MB test frame
            
            uint64_t timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            
            bool success = protocol_.send_video_frame(
                frame_data, 
                protocol::FrameType::P_FRAME, 
                timestamp
            );
            
            if (success) {
                std::cout << "Frame " << frame << " sent successfully" << std::endl;
            } else {
                std::cout << "Failed to send frame " << frame << std::endl;
            }
            
            // Simulate 30 FPS
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
        }
    }
    
    void test_audio_streaming() {
        std::cout << "🎵 Testing audio streaming..." << std::endl;
        
        // Simulate audio frames
        for (int chunk = 0; chunk < 300; ++chunk) {
            std::vector<uint8_t> audio_data(1024, 0xBB); // 1KB audio chunk
            
            uint64_t timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            
            bool success = protocol_.send_audio_frame(
                audio_data,
                48000,  // 48kHz
                2,      // Stereo
                timestamp
            );
            
            if (chunk % 100 == 0) {
                std::cout << "Audio chunk " << chunk << " sent" << std::endl;
            }
            
            // Simulate audio packets (every 10ms)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    void print_statistics() {
        auto stats = protocol_.get_statistics();
        
        std::cout << "\n📊 Protocol Statistics:" << std::endl;
        std::cout << "=======================" << std::endl;
        std::cout << "Packets Sent: " << stats.packets_sent << std::endl;
        std::cout << "Packets Received: " << stats.packets_received << std::endl;
        std::cout << "Packets Lost: " << stats.packets_lost << std::endl;
        std::cout << "Current Bitrate: " << stats.current_bitrate / 1000 << " Kbps" << std::endl;
        std::cout << "Current RTT: " << stats.current_rtt << " ms" << std::endl;
        std::cout << "Packet Loss: " << stats.current_packet_loss * 100 << " %" << std::endl;
        std::cout << "Queue Latency: " << stats.queue_latency_ms << " ms" << std::endl;
    }
};

int main() {
    try {
        ProtocolTester tester;
        
        if (!tester.initialize()) {
            std::cerr << "Failed to initialize protocol tester" << std::endl;
            return 1;
        }
        
        std::cout << "🚀 Streaming Protocol Test" << std::endl;
        std::cout << "=========================" << std::endl;
        
        // Test video streaming
        tester.test_video_streaming();
        
        // Test audio streaming
        tester.test_audio_streaming();
        
        // Print results
        tester.print_statistics();
        
        std::cout << "\n✅ Protocol test completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
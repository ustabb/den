// examples/player_example.cpp
#include "streaming/client/streaming_client.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace streaming;

class PlayerTester {
private:
    client::StreamingClient player_;
    
public:
    bool initialize_player() {
        client::StreamingClient::ClientConfig config;
        config.video_width = 1280;
        config.video_height = 720;
        config.target_fps = 60;
        config.hardware_acceleration = true;
        config.low_latency_mode = true;
        config.buffer_duration_ms = 2000;
        
        // Set up callbacks
        player_.set_state_changed_callback([this](auto old_state, auto new_state) {
            on_state_changed(old_state, new_state);
        });
        
        player_.set_error_callback([this](const std::string& error) {
            on_error(error);
        });
        
        player_.set_statistics_callback([this](const client::StreamingClient::PlayerState& stats) {
            on_statistics(stats);
        });
        
        if (!player_.initialize(config)) {
            std::cerr << "Failed to initialize player" << std::endl;
            return false;
        }
        
        return true;
    }
    
    void play_stream(const std::string& url) {
        std::cout << "🎬 Playing stream: " << url << std::endl;
        player_.play(url);
    }
    
    void on_state_changed(client::StreamingClient::PlayerState::State old_state, 
                         client::StreamingClient::PlayerState::State new_state) {
        const char* state_names[] = {"STOPPED", "CONNECTING", "BUFFERING", "PLAYING", "PAUSED", "ERROR"};
        std::cout << "🔀 State changed: " << state_names[old_state] 
                  << " -> " << state_names[new_state] << std::endl;
    }
    
    void on_error(const std::string& error) {
        std::cerr << "❌ Player error: " << error << std::endl;
    }
    
    void on_statistics(const client::StreamingClient::PlayerState& stats) {
        std::cout << "📊 Stats - Time: " << stats.current_time 
                  << "s, Frames: " << stats.frames_decoded
                  << ", Dropped: " << stats.frames_dropped
                  << ", Bitrate: " << stats.current_bitrate / 1000 << " kbps"
                  << ", Latency: " << stats.network_latency << "ms" << std::endl;
    }
    
    void run_test() {
        std::cout << "🚀 Streaming Player Test" << std::endl;
        std::cout << "========================" << std::endl;
        
        // Test different stream types
        std::vector<std::string> test_streams = {
            "http://localhost:8080/stream.flv",
            "http://localhost:8080/stream.m3u8",
            "rtmp://localhost:1935/live/stream"
        };
        
        for (const auto& stream_url : test_streams) {
            std::cout << "\nTesting: " << stream_url << std::endl;
            play_stream(stream_url);
            
            // Play for 10 seconds
            std::this_thread::sleep_for(std::chrono::seconds(10));
            player_.stop();
            
            std::this_thread::sleep_for(std::chrono::seconds(2)); // Cooldown
        }
    }
};

int main() {
    try {
        PlayerTester tester;
        
        if (!tester.initialize_player()) {
            std::cerr << "Failed to initialize player tester" << std::endl;
            return 1;
        }
        
        tester.run_test();
        
        std::cout << "\n✅ Player test completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
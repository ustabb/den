// examples/video_streaming_example.cpp
#include "streaming/video/frame_processor.hpp"
#include "streaming/network/socket_manager.hpp"
#include <iostream>

using namespace streaming;

int main() {
    try {
        // Streaming konfigürasyonu
        StreamConfig config;
        config.host = "192.168.1.100";
        config.port = 8080;
        config.low_latency_mode = true;
        config.zero_copy_enabled = true;
        
        // Video processing pipeline
        video::FrameProcessor processor;
        if (!processor.initialize(config)) {
            std::cerr << "Frame processor initialization failed!" << std::endl;
            return -1;
        }
        
        // Output callback - encoded paketleri network'e gönder
        processor.set_output_callback([&config](const uint8_t* data, size_t size) {
            // Burada socket manager ile gönderim yapılacak
            std::cout << "Encoded packet ready: " << size << " bytes" << std::endl;
        });
        
        std::cout << "🚀 Starting video streaming engine..." << std::endl;
        processor.start_processing();
        
        // Simülasyon: 10 saniye çalıştır
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        processor.stop_processing();
        std::cout << "✅ Streaming completed!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cer
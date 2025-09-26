// examples/custom_codec_example.cpp
#include "streaming/codec/h264_encoder.hpp"
#include <iostream>

using namespace streaming;

int main() {
    try {
        codec::H264Encoder encoder;
        
        // Encoder'ı başlat
        if (!encoder.initialize(1920, 1080, 30, 4000000)) { // 4Mbps
            std::cerr << "Encoder initialization failed!" << std::endl;
            return -1;
        }
        
        // Test frame oluştur
        codec::VideoFrame frame;
        frame.width = 1920;
        frame.height = 1080;
        frame.data.resize(1920 * 1080 * 3, 128); // Gray frame
        
        // Frame'i encode et
        std::vector<uint8_t> encoded_data;
        if (encoder.encode_frame(frame, encoded_data)) {
            std::cout << "✅ Frame encoded successfully! Size: " 
                      << encoded_data.size() << " bytes" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
// examples/h264_encoder_test.cpp
#include "streaming/codec/h264_encoder.hpp"
#include "streaming/codec/h264_decoder.hpp"
#include <chrono>
#include <fstream>

using namespace streaming;

int main() {
    try {
        // Encoder'ı başlat
        codec::H264Encoder encoder;
        if (!encoder.initialize(1920, 1080, 30, 4000000)) {
            std::cerr << "Encoder initialization failed!" << std::endl;
            return -1;
        }
        
        // Test frame oluştur (gradient pattern)
        codec::VideoFrame frame;
        frame.width = 1920;
        frame.height = 1080;
        frame.data.resize(1920 * 1080);
        
        for (int y = 0; y < 1080; ++y) {
            for (int x = 0; x < 1920; ++x) {
                frame.data[y * 1920 + x] = static_cast<uint8_t>((x + y) % 256);
            }
        }
        
        std::cout << "🎥 Encoding test frame..." << std::endl;
        
        // Encode et
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<uint8_t> encoded_data;
        if (!encoder.encode_frame(frame, encoded_data)) {
            std::cerr << "Encoding failed!" << std::endl;
            return -1;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "✅ Encoding successful!" << std::endl;
        std::cout << "Original size: " << (1920 * 1080) << " bytes" << std::endl;
        std::cout << "Encoded size: " << encoded_data.size() << " bytes" << std::endl;
        std::cout << "Compression ratio: " << (1920.0 * 1080 / encoded_data.size()) << ":1" << std::endl;
        std::cout << "Encoding time: " << duration.count() << " μs" << std::endl;
        
        // Encoded datayı dosyaya yaz (analiz için)
        std::ofstream file("encoded_frame.h264", std::ios::binary);
        file.write(reinterpret_cast<const char*>(encoded_data.data()), encoded_data.size());
        file.close();
        
        std::cout << "💾 Encoded data saved to encoded_frame.h264" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
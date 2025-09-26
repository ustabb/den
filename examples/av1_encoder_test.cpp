// examples/av1_encoder_test.cpp
#include "streaming/codec/av1_encoder.hpp"
#include <chrono>
#include <fstream>

using namespace streaming;

int main() {
    try {
        // AV1 Encoder'ı başlat
        codec::AV1Encoder encoder;
        
        // AV1 tool'larını etkinleştir
        encoder.enable_tools(true, true, true, false);
        encoder.set_speed_preset(5); // Balanced preset
        
        if (!encoder.initialize(1920, 1080, 30, 2500000)) { // 2.5Mbps
            std::cerr << "AV1 Encoder initialization failed!" << std::endl;
            return -1;
        }
        
        // Test frame oluştur (AV1'in güçlü yönlerini gösteren pattern)
        codec::VideoFrame frame;
        frame.width = 1920;
        frame.height = 1080;
        frame.data.resize(1920 * 1080);
        
        // Karmaşık test pattern - AV1'in sıkıştırma avantajını gösterir
        for (int y = 0; y < 1080; ++y) {
            for (int x = 0; x < 1920; ++x) {
                // Çeşitli frekanslarda patternler
                double pattern1 = std::sin(x * 0.02) * std::cos(y * 0.015) * 64;
                double pattern2 = std::sin((x + y) * 0.01) * 32;
                double pattern3 = (x % 64 < 32 && y % 64 < 32) ? 16 : -16;
                double pattern4 = ((x * y) % 512) / 2.0;
                
                int value = 128 + pattern1 + pattern2 + pattern3 + pattern4;
                frame.data[y * 1920 + x] = static_cast<uint8_t>(std::max(0, std::min(255, value)));
            }
        }
        
        std::cout << "🎥 AV1 Encoding complex test frame..." << std::endl;
        std::cout << "   Using OBMC: Yes, CFL: Yes, Palette: Yes" << std::endl;
        
        // Encode et
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<uint8_t> encoded_data;
        if (!encoder.encode_frame(frame, encoded_data)) {
            std::cerr << "AV1 Encoding failed!" << std::endl;
            return -1;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "✅ AV1 Encoding successful!" << std::endl;
        std::cout << "Original size: " << (1920 * 1080) << " bytes" << std::endl;
        std::cout << "Encoded size: " << encoded_data.size() << " bytes" << std::endl;
        std::cout << "Compression ratio: " << (1920.0 * 1080 / encoded_data.size()) << ":1" << std::endl;
        std::cout << "Encoding time: " << duration.count() << " μs" << std::endl;
        std::cout << "Bitrate: " << (encoded_data.size() * 8 * 30 / 1000000.0) << " Mbps" << std::endl;
        
        // AV1'in H.265'e göre avantajını göster
        std::cout << "🏆 AV1 typically provides 20-30% better compression than H.265" << std::endl;
        
        // Encoded datayı dosyaya yaz
        std::ofstream file("encoded_frame.av1", std::ios::binary);
        file.write(reinterpret_cast<const char*>(encoded_data.data()), encoded_data.size());
        file.close();
        
        std::cout << "💾 AV1 encoded data saved to encoded_frame.av1" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
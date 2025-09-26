// examples/h265_encoder_test.cpp
#include "streaming/codec/h265_encoder.hpp"
#include <chrono>
#include <fstream>

using namespace streaming;

int main() {
    try {
        // H.265 Encoder'ı başlat
        codec::H265Encoder encoder;
        if (!encoder.initialize(1920, 1080, 30, 3000000)) { // 3Mbps
            std::cerr << "H.265 Encoder initialization failed!" << std::endl;
            return -1;
        }
        
        // Test frame oluştur
        codec::VideoFrame frame;
        frame.width = 1920;
        frame.height = 1080;
        frame.data.resize(1920 * 1080);
        
        // Daha karmaşık test pattern (HEVC'in avantajlarını göstermek için)
        for (int y = 0; y < 1080; ++y) {
            for (int x = 0; x < 1920; ++x) {
                // Çeşitli patternler
                uint8_t value = static_cast<uint8_t>(
                    (std::sin(x * 0.01) * 127 + 128) * 0.3 +
                    (std::cos(y * 0.005) * 127 + 128) * 0.3 +
                    ((x * y) % 256) * 0.4
                );
                frame.data[y * 1920 + x] = value;
            }
        }
        
        std::cout << "🎥 H.265 Encoding test frame..." << std::endl;
        
        // Encode et
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<uint8_t> encoded_data;
        if (!encoder.encode_frame(frame, encoded_data)) {
            std::cerr << "H.265 Encoding failed!" << std::endl;
            return -1;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "✅ H.265 Encoding successful!" << std::endl;
        std::cout << "Original size: " << (1920 * 1080) << " bytes" << std::endl;
        std::cout << "Encoded size: " << encoded_data.size() << " bytes" << std::endl;
        std::cout << "Compression ratio: " << (1920.0 * 1080 / encoded_data.size()) << ":1" << std::endl;
        std::cout << "Encoding time: " << duration.count() << " μs" << std::endl;
        std::cout << "Bitrate: " << (encoded_data.size() * 8 * 30 / 1000000.0) << " Mbps" << std::endl;
        
        // Encoded datayı dosyaya yaz
        std::ofstream file("encoded_frame.h265", std::ios::binary);
        file.write(reinterpret_cast<const char*>(encoded_data.data()), encoded_data.size());
        file.close();
        
        std::cout << "💾 H.265 encoded data saved to encoded_frame.h265" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
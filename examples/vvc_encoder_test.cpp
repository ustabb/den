// examples/vvc_encoder_test.cpp
#include "streaming/codec/vvc_encoder.hpp"
#include <chrono>
#include <fstream>

using namespace streaming;

int main() {
    try {
        // VVC Encoder'ı başlat
        codec::VVCEncoder encoder;
        
        // VVC advanced tools'ları etkinleştir
        codec::VVCAdvancedFeatures features;
        features.mip_enabled = true;
        features.affine_enabled = true;
        features.ibc_enabled = true;
        features.cclm_enabled = true;
        
        encoder.enable_advanced_tools(features);
        encoder.set_complexity_level(7); // High quality
        encoder.set_parallel_processing(true);
        
        if (!encoder.initialize(1920, 1080, 30, 2000000)) { // 2Mbps - H.265'ten daha az!
            std::cerr << "VVC Encoder initialization failed!" << std::endl;
            return -1;
        }
        
        // Test frame oluştur - VVC'nin güçlü yönlerini göster
        codec::VideoFrame frame;
        frame.width = 1920;
        frame.height = 1080;
        frame.data.resize(1920 * 1080);
        
        // VVC için optimize edilmiş test pattern
        // Screen content + natural video mix
        for (int y = 0; y < 1080; ++y) {
            for (int x = 0; x < 1920; ++x) {
                // Natural video component
                double natural = std::sin(x * 0.03) * std::cos(y * 0.02) * 48;
                
                // Screen content component (IBC için ideal)
                double screen = ((x / 32) % 2 == (y / 32) % 2) ? 32 : -32;
                
                // High-frequency details (MIP için test)
                double details = std::sin(x * 0.5) * std::cos(y * 0.3) * 16;
                
                int value = 128 + natural + screen + details;
                frame.data[y * 1920 + x] = static_cast<uint8_t>(std::max(0, std::min(255, value)));
            }
        }
        
        std::cout << "🎥 VVC Encoding advanced test frame..." << std::endl;
        std::cout << "   Using MIP: Yes, Affine: Yes, IBC: Yes, Parallel: Yes" << std::endl;
        std::cout << "   Target: 2Mbps (50% less than typical H.265)" << std::endl;
        
        // Encode et
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<uint8_t> encoded_data;
        if (!encoder.encode_frame(frame, encoded_data)) {
            std::cerr << "VVC Encoding failed!" << std::endl;
            return -1;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "✅ VVC Encoding successful!" << std::endl;
        std::cout << "Original size: " << (1920 * 1080) << " bytes" << std::endl;
        std::cout << "Encoded size: " << encoded_data.size() << " bytes" << std::endl;
        std::cout << "Compression ratio: " << (1920.0 * 1080 / encoded_data.size()) << ":1" << std::endl;
        std::cout << "Encoding time: " << duration.count() << " μs" << std::endl;
        std::cout << "Actual bitrate: " << (encoded_data.size() * 8 * 30 / 1000000.0) << " Mbps" << std::endl;
        
        // VVC'nin avantajlarını göster
        std::cout << "🏆 VVC provides 50% better compression than H.265" << std::endl;
        std::cout << "🎯 Advanced features:" << std::endl;
        std::cout << "   - MTT partitioning (QT+BT+TT)" << std::endl;
        std::cout << "   - 256x256 CTU support" << std::endl;
        std::cout << "   - Affine motion prediction" << std::endl;
        std::cout << "   - Intra Block Copy (IBC)" << std::endl;
        std::cout << "   - Matrix-based Intra Prediction (MIP)" << std::endl;
        
        // Encoded datayı dosyaya yaz
        std::ofstream file("encoded_frame.vvc", std::ios::binary);
        file.write(reinterpret_cast<const char*>(encoded_data.data()), encoded_data.size());
        file.close();
        
        std::cout << "💾 VVC encoded data saved to encoded_frame.vvc" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
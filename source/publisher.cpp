#include "streaming/publisher.hpp"
#include <spdlog/spdlog.h>

using namespace streaming;

Publisher::Publisher(std::unique_ptr<media::Encoder> encoder, std::unique_ptr<network::UDPServer> server)
    : encoder_(std::move(encoder)), server_(std::move(server)) {
}

bool Publisher::publish_frame(const std::vector<uint8_t>& frame_data) {
    if (!encoder_ || !server_) {
        spdlog::error("Publisher not properly initialized");
        return false;
    }
    
    try {
        auto encoded = encoder_->encode(frame_data);
        if (encoded.empty()) {
            spdlog::warn("Encoded frame is empty");
            return false;
        }
        
        // Örnek hedef adres - gerçek uygulamada config'ten alınmalı
        bool sent = server_->send("127.0.0.1", 8080, encoded);
        if (sent) {
            spdlog::info("Frame published, encoded size: {} bytes", encoded.size());
        } else {
            spdlog::error("Failed to send encoded frame");
        }
        return sent;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to encode frame: {}", e.what());
        return false;
    }
}

bool Publisher::publish_frame(const media::RawFrame& frame) {
    return publish_frame(frame.data);
}
#include "streaming/subscriber.hpp"
#include <spdlog/spdlog.h>

using namespace streaming;

Subscriber::Subscriber(std::unique_ptr<media::Decoder> decoder, std::unique_ptr<network::UDPServer> server)
    : decoder_(std::move(decoder)), server_(std::move(server)) {
}

std::vector<uint8_t> Subscriber::receive_frame() {
    if (!decoder_ || !server_) {
        spdlog::error("Subscriber not properly initialized");
        return {};
    }
    
    try {
        auto received_data = server_->receive();
        if (received_data.empty()) {
            return {};
        }
        
        auto decoded = decoder_->decode(received_data);
        spdlog::info("Frame received and decoded, size: {} bytes", decoded.size());
        return decoded;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to decode frame: {}", e.what());
        return {};
    }
}

media::RawFrame Subscriber::receive_frame_as_raw() {
    auto decoded = receive_frame();
    return media::RawFrame::from_vector(decoded);
}
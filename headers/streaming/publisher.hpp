#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include "media/encoder.hpp"
#include "network/udp_server.hpp"

namespace streaming {

class Publisher {
private:
    std::unique_ptr<media::Encoder> encoder_;
    std::unique_ptr<network::UDPServer> server_;

public:
    Publisher(std::unique_ptr<media::Encoder> encoder, std::unique_ptr<network::UDPServer> server);
    
    bool publish_frame(const std::vector<uint8_t>& frame_data);
    bool publish_frame(const media::RawFrame& frame);
};

} // namespace streaming
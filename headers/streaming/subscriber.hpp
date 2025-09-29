#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include "media/decoder.hpp"
#include "network/udp_server.hpp"

namespace streaming {

class Subscriber {
private:
    std::unique_ptr<media::Decoder> decoder_;
    std::unique_ptr<network::UDPServer> server_;

public:
    Subscriber(std::unique_ptr<media::Decoder> decoder, std::unique_ptr<network::UDPServer> server);
    
    std::vector<uint8_t> receive_frame();
    media::RawFrame receive_frame_as_raw();
};

} // namespace streaming
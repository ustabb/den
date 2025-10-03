// headers/network/protocol.hpp
#pragma once

#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include <string>

namespace network {

using tcp = boost::asio::ip::tcp;

// Very small enum to represent detected protocol
enum class DetectedProtocol {
    Unknown,
    RTMP,
    HTTP,      // includes HTTP-FLV, HLS playlist requests, etc.
    RTSP,
    WebSocket,
    Other
};

// ProtocolRouter: analyze initial bytes and route socket to proper handler
// NOTE: real handler functions should be implemented in their own modules
class ProtocolRouter {
public:
    // Route the connection and initial bytes to the appropriate handler.
    // Ownership of socket is transferred to handler (shared_ptr).
    static void route(std::shared_ptr<tcp::socket> sock, const std::vector<uint8_t>& initial_data);

    // Helper: detect protocol from initial payload (may be empty)
    static DetectedProtocol detect_protocol(const std::vector<uint8_t>& initial_data);
};

} // namespace network

// source/network/protocol.cpp
#include "network/protocol.hpp"
#include "network/http_session.hpp"
#include <spdlog/spdlog.h>
#include <string>
#include <algorithm>

using namespace network;

DetectedProtocol ProtocolRouter::detect_protocol(const std::vector<uint8_t>& initial_data) {
    if (initial_data.empty()) return DetectedProtocol::Unknown;

    // Convert to string for ASCII based checks (like HTTP/RTSP)
    std::string s(reinterpret_cast<const char*>(initial_data.data()), initial_data.size());

    // Trim leading whitespace
    auto it = std::find_if_not(s.begin(), s.end(), [](char c){ return c == ' ' || c == '\r' || c == '\n' || c == '\t'; });
    std::string prefix = (it == s.end()) ? std::string() : std::string(it, s.end());

    // Check for RTMP C0: first byte is 0x03 (version)
    if (!initial_data.empty() && initial_data[0] == 0x03) {
        spdlog::debug("ProtocolDetector: likely RTMP (C0 0x03)");
        return DetectedProtocol::RTMP;
    }

    // HTTP methods (GET, POST, OPTIONS, HEAD)
    if (prefix.rfind("GET ", 0) == 0 || prefix.rfind("POST ", 0) == 0 ||
        prefix.rfind("OPTIONS ", 0) == 0 || prefix.rfind("HEAD ", 0) == 0) {
        spdlog::debug("ProtocolDetector: HTTP-based protocol detected");
        return DetectedProtocol::HTTP;
    }

    // RTSP tends to start with "OPTIONS", "DESCRIBE", "SETUP", "PLAY"
    if (prefix.rfind("OPTIONS ",0) == 0 || prefix.rfind("DESCRIBE ",0) == 0 ||
        prefix.rfind("SETUP ",0) == 0 || prefix.rfind("PLAY ",0) == 0) {
        spdlog::debug("ProtocolDetector: RTSP detected");
        return DetectedProtocol::RTSP;
    }

    // WebSocket handshake begins with HTTP upgrade header but still HTTP
    if (prefix.rfind("GET ",0) == 0 && s.find("Upgrade: websocket") != std::string::npos) {
        spdlog::debug("ProtocolDetector: WebSocket detected");
        return DetectedProtocol::WebSocket;
    }

    // If first byte looks printable ASCII, but no known prefix -> assume HTTP/Other
    if (std::all_of(initial_data.begin(), initial_data.end(), [](uint8_t c){ return (c >= 32 && c <= 126) || c == '\r' || c == '\n' || c == '\t'; })) {
        spdlog::debug("ProtocolDetector: ASCII printable initial data -> HTTP/Other");
        return DetectedProtocol::HTTP;
    }

    spdlog::debug("ProtocolDetector: Unknown protocol");
    return DetectedProtocol::Unknown;
}

void ProtocolRouter::route(std::shared_ptr<tcp::socket> sock, const std::vector<uint8_t>& initial_data) {
    auto proto = detect_protocol(initial_data);
    try {
        switch (proto) {
            case DetectedProtocol::RTMP:
                spdlog::info("ProtocolRouter: Routing to RTMP handler for {}", sock->remote_endpoint().address().to_string());
                // TODO: rtmp::handle_connection(sock);
                sock->close();
                break;

            case DetectedProtocol::HTTP:
                spdlog::info("ProtocolRouter: Routing to HTTP handler for {}", sock->remote_endpoint().address().to_string());
                std::make_shared<HttpSession>(sock)->start();
                break;

            case DetectedProtocol::RTSP:
                spdlog::info("ProtocolRouter: Routing to RTSP handler for {}", sock->remote_endpoint().address().to_string());
                // TODO: rtsp::handle_connection(sock);
                sock->close();
                break;

            case DetectedProtocol::WebSocket:
                spdlog::info("ProtocolRouter: Routing to WebSocket handler for {}", sock->remote_endpoint().address().to_string());
                // TODO: ws::handle_connection(sock);
                sock->close();
                break;

            default:
                spdlog::warn("ProtocolRouter: Unknown protocol from {}, closing socket", sock->remote_endpoint().address().to_string());
                sock->close();
                break;
        }
    } catch (const std::exception& ex) {
        spdlog::error("ProtocolRouter: exception while routing: {}", ex.what());
        boost::system::error_code ec;
        sock->close(ec);
    }
}

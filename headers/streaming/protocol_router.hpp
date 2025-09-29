#pragma once
// ProtocolRouter: Routes frames to correct protocol handler (RTMP/HLS/WebRTC)
namespace streaming {
class ProtocolRouter {
public:
	ProtocolRouter();
	void route(const std::vector<uint8_t>& frame, int protocolId);
	// Add protocol selection and routing logic here
};
}

#pragma once
// Protocol: Defines supported streaming protocols (RTMP/HLS/WebRTC)
namespace network {
enum class ProtocolType {
	RTMP,
	HLS,
	WebRTC
};
}

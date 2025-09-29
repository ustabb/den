#pragma once
// TCPServer: Accepts and manages TCP connections for streaming
namespace network {
class TCPServer {
public:
	TCPServer(int port);
	void start();
	void stop();
	// Add connection and packet handling here
};
}

#pragma once
// API server: REST/gRPC endpoint definitions and runtime controls
namespace control {
class APIServer {
public:
	APIServer();
	void start();
	void stop();
	// Add endpoint registration and request handling here
};
}

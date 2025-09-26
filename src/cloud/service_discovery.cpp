// include/streaming/cloud/service_discovery.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace streaming {
namespace cloud {

class ServiceDiscovery {
public:
    struct ServiceEndpoint {
        std::string service_name;
        std::string ip_address;
        uint16_t port;
        std::string protocol; // http, https, rtmp, etc.
        std::string region;
        std::string zone;
        double current_load; // 0.0 to 1.0
        uint32_t active_connections;
        bool healthy = true;
        uint64_t last_health_check;
    };

    struct LoadBalancingStrategy {
        enum class Strategy {
            ROUND_ROBIN,
            LEAST_CONNECTIONS,
            LEAST_LOAD,
            LATENCY_BASED,
            GEOGRAPHIC
        };
        
        Strategy strategy = Strategy::LEAST_LOAD;
        uint32_t weight = 100;
        std::string custom_parameters;
    };

    ServiceDiscovery();
    ~ServiceDiscovery();
    
    bool register_service(const ServiceEndpoint& endpoint);
    bool deregister_service(const std::string& service_name, const std::string& ip);
    
    ServiceEndpoint discover_service(const std::string& service_name, 
                                   const LoadBalancingStrategy& strategy = {});
    std::vector<ServiceEndpoint> discover_all_services(const std::string& service_name);
    
    // Health checking
    void start_health_checks();
    void stop_health_checks();
    bool is_service_healthy(const std::string& service_name, const std::string& ip);
    
    // Advanced features
    bool enable_graceful_shutdown(const std::string& service_name, const std::string& ip);
    bool drain_connections(const std::string& service_name, const std::string& ip);

private:
    void health_check_loop();
    ServiceEndpoint round_robin_selection(const std::vector<ServiceEndpoint>& endpoints);
    ServiceEndpoint least_connections_selection(const std::vector<ServiceEndpoint>& endpoints);
    ServiceEndpoint least_load_selection(const std::vector<ServiceEndpoint>& endpoints);
    ServiceEndpoint latency_based_selection(const std::vector<ServiceEndpoint>& endpoints);
    
    std::unordered_map<std::string, std::vector<ServiceEndpoint>> service_registry_;
    mutable std::mutex registry_mutex_;
    std::atomic<bool> health_checking_active_{false};
    std::thread health_check_thread_;
};

} // namespace cloud
} // namespace streaming
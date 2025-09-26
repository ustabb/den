// include/streaming/cloud/cloud_manager.hpp
#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

namespace streaming {
namespace cloud {

class CloudManager {
public:
    enum class CloudProvider {
        AWS,
        GOOGLE_CLOUD,
        AZURE,
        MULTI_CLOUD
    };

    struct CloudConfig {
        CloudProvider provider;
        std::string region;
        std::string cluster_name;
        uint32_t min_nodes = 2;
        uint32_t max_nodes = 10;
        std::string instance_type = "c5.2xlarge";
        bool auto_scaling = true;
        bool multi_zone = true;
        std::string storage_class = "gp3";
    };

    struct DeploymentStatus {
        std::string deployment_id;
        std::string status; // RUNNING, SCALING, ERROR
        uint32_t current_replicas;
        uint32_t desired_replicas;
        double cpu_utilization;
        double memory_utilization;
        std::vector<std::string> active_pods;
        std::string load_balancer_ip;
    };

    CloudManager();
    ~CloudManager();
    
    bool initialize(const CloudConfig& config);
    DeploymentStatus deploy_application(const std::string& app_name, 
                                      const std::string& docker_image,
                                      const std::unordered_map<std::string, std::string>& env_vars);
    
    bool scale_application(const std::string& app_name, uint32_t replicas);
    bool update_application(const std::string& app_name, const std::string& new_image);
    bool delete_application(const std::string& app_name);
    
    // Multi-cloud management
    bool deploy_multi_cloud(const std::vector<CloudConfig>& configs);
    bool enable_cross_cloud_load_balancing();
    bool sync_configurations_across_clouds();
    
    // Disaster recovery
    bool create_backup(const std::string& app_name);
    bool restore_from_backup(const std::string& app_name, const std::string& backup_id);
    bool failover_to_backup_region(const std::string& primary_region);

    // Monitoring and metrics
    DeploymentStatus get_deployment_status(const std::string& app_name);
    std::vector<DeploymentStatus> get_all_deployments();
    bool set_up_monitoring(const std::string& app_name);

private:
    // Cloud-specific implementations
    bool initialize_aws();
    bool initialize_google_cloud();
    bool initialize_azure();
    
    DeploymentStatus deploy_to_aws(const std::string& app_name, const std::string& image);
    DeploymentStatus deploy_to_google_cloud(const std::string& app_name, const std::string& image);
    DeploymentStatus deploy_to_azure(const std::string& app_name, const std::string& image);
    
    CloudConfig config_;
    std::unordered_map<std::string, DeploymentStatus> deployments_;
};

} // namespace cloud
} // namespace streaming
// include/streaming/cloud/auto_scaler.hpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>

namespace streaming {
namespace cloud {

class AutoScaler {
public:
    struct ScalingMetric {
        std::string name;
        double current_value;
        double target_value;
        double threshold_high;
        double threshold_low;
    };

    struct ScalingPolicy {
        std::string policy_name;
        uint32_t min_replicas;
        uint32_t max_replicas;
        uint32_t cooldown_period_seconds = 300;
        std::vector<ScalingMetric> metrics;
        std::string scaling_algorithm = "proportional";
    };

    struct ScalingDecision {
        bool should_scale;
        int32_t replica_delta; // Positive for scale-out, negative for scale-in
        std::string reason;
        double confidence;
        std::vector<ScalingMetric> triggering_metrics;
    };

    AutoScaler();
    ~AutoScaler();
    
    void add_scaling_policy(const ScalingPolicy& policy);
    ScalingDecision evaluate_scaling_needs(const std::vector<ScalingMetric>& current_metrics);
    
    // Advanced scaling algorithms
    ScalingDecision proportional_scaling(const std::vector<ScalingMetric>& metrics, 
                                        const ScalingPolicy& policy);
    ScalingDecision predictive_scaling(const std::vector<ScalingMetric>& metrics,
                                      const ScalingPolicy& policy);
    ScalingDecision reinforcement_learning_scaling(const std::vector<ScalingMetric>& metrics,
                                                  const ScalingPolicy& policy);
    
    // Machine learning-based scaling
    void train_scaling_model(const std::vector<std::vector<ScalingMetric>>& historical_data);
    double predict_future_load(uint32_t minutes_ahead);

private:
    double calculate_combined_metric(const std::vector<ScalingMetric>& metrics);
    bool is_in_cooldown_period(const std::string& policy_name);
    void update_cooldown_timer(const std::string& policy_name);
    
    std::vector<ScalingPolicy> policies_;
    std::unordered_map<std::string, uint64_t> cooldown_timers_; // policy_name -> cooldown_end_time
    uint64_t last_prediction_time_ = 0;
};

} // namespace cloud
} // namespace streaming
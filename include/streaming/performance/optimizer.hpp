// include/streaming/performance/optimizer.hpp
#pragma once

#include "profiler.hpp"
#include "cache_optimizer.hpp"
#include "vectorization.hpp"
#include "memory_pool.hpp"
#include "parallelization.hpp"
#include <unordered_map>
#include <string>

namespace streaming {
namespace performance {

class PerformanceOptimizer {
public:
    struct OptimizationResult {
        std::string component;
        double original_time_ms;
        double optimized_time_ms;
        double improvement_ratio;
        std::string optimization_applied;
        std::vector<std::string> recommendations;
    };

    struct SystemMetrics {
        double cpu_usage;
        double memory_usage;
        double cache_hit_rate;
        double branch_prediction_accuracy;
        double vectorization_utilization;
        size_t context_switches;
    };

    PerformanceOptimizer();
    
    void analyze_performance();
    OptimizationResult optimize_component(const std::string& component_name);
    SystemMetrics get_system_metrics();
    
    // Automatic optimization
    void enable_auto_optimization(bool enable);
    void set_optimization_target(const std::string& target); // "speed", "memory", "balanced"
    
    // Runtime adaptation
    void adapt_to_current_load();
    void optimize_for_battery_mode();
    void optimize_for_high_performance();

    // Profile-guided optimization
    void load_profile_data(const std::string& filename);
    void save_profile_data(const std::string& filename);
    void apply_profile_guided_optimizations();

private:
    OptimizationResult optimize_encoding_pipeline();
    OptimizationResult optimize_network_stack();
    OptimizationResult optimize_memory_usage();
    OptimizationResult optimize_cache_behavior();
    
    void apply_vectorization_optimizations();
    void apply_memory_pool_optimizations();
    void apply_parallelization_optimizations();
    void apply_cache_optimizations();
    
    HighResProfiler profiler_;
    CacheOptimizer cache_optimizer_;
    SIMDVectorizer vectorizer_;
    
    std::unordered_map<std::string, OptimizationResult> optimization_history_;
    std::string current_optimization_target_ = "speed";
    bool auto_optimization_enabled_ = true;
};

} // namespace performance
} // namespace streaming
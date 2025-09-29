// src/performance/real_time_monitor.cpp
#include "streaming/performance/optimizer.hpp"
#include "streaming/utils/logger.hpp"
#include <chrono>
#include <thread>

namespace streaming {
namespace performance {

class RealTimePerformanceMonitor {
public:
    void start_monitoring() {
        monitor_thread_ = std::thread([this]() {
            while (monitoring_) {
                auto metrics = optimizer_.get_system_metrics();
                check_performance_anomalies(metrics);
                suggest_optimizations(metrics);
                
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        });
    }
    
    void check_performance_anomalies(const PerformanceOptimizer::SystemMetrics& metrics) {
        if (metrics.cpu_usage > 90.0) {
            LOG_WARN("High CPU usage detected: {:.1f}%", metrics.cpu_usage);
            optimizer_.optimize_for_battery_mode();
        }
        
        if (metrics.cache_hit_rate < 80.0) {
            LOG_WARN("Low cache hit rate: {:.1f}%", metrics.cache_hit_rate);
            optimizer_.apply_cache_optimizations();
        }
        
        if (metrics.vectorization_utilization < 50.0) {
            LOG_WARN("Low vectorization utilization: {:.1f}%", metrics.vectorization_utilization);
            optimizer_.apply_vectorization_optimizations();
        }
    }

private:
    PerformanceOptimizer optimizer_;
    std::thread monitor_thread_;
    std::atomic<bool> monitoring_{false};
};

} // namespace performance
} // namespace streaming
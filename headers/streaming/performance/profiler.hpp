// include/streaming/performance/profiler.hpp
#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <stack>
#include <atomic>

namespace streaming {
namespace performance {

class HighResProfiler {
public:
    struct ProfileResult {
        std::string name;
        uint64_t start_time;
        uint64_t end_time;
        uint64_t thread_id;
        uint32_t call_count = 1;
        uint64_t total_time = 0;
        uint64_t min_time = UINT64_MAX;
        uint64_t max_time = 0;
        uint32_t memory_usage = 0;
        uint32_t cache_misses = 0;
        uint32_t branch_mispredicts = 0;
    };

    struct ThreadData {
        std::stack<ProfileResult*> call_stack;
        std::vector<ProfileResult> results;
        uint32_t depth = 0;
    };

    HighResProfiler();
    ~HighResProfiler();
    
    void start_session(const std::string& name = "Session");
    void end_session();
    
    void begin_sample(const std::string& name);
    void end_sample();
    
    // Advanced profiling
    void record_memory_usage(size_t bytes);
    void record_cache_misses(uint32_t misses);
    void record_branch_mispredicts(uint32_t mispredicts);
    void record_cpu_cycles(uint64_t cycles);
    
    // Real-time statistics
    double get_average_time(const std::string& sample_name) const;
    double get_throughput(const std::string& sample_name) const; // operations/sec
    uint32_t get_call_count(const std::string& sample_name) const;
    
    // Export results
    void export_to_chrome_tracing(const std::string& filename);
    void export_to_csv(const std::string& filename);
    void print_summary() const;

    // Singleton access
    static HighResProfiler& get_instance();

private:
    uint64_t get_current_time_ns() const;
    ProfileResult* get_or_create_result(const std::string& name);
    void update_statistics(ProfileResult& result, uint64_t duration);
    
    std::unordered_map<std::string, ProfileResult> results_;
    std::unordered_map<std::thread::id, ThreadData> thread_data_;
    mutable std::mutex mutex_;
    std::string current_session_;
    bool session_active_ = false;
    
    // Performance counters
    std::atomic<uint64_t> total_samples_{0};
    std::atomic<uint64_t> total_duration_{0};
};

// RAII-based profiling macro
class ScopedProfiler {
public:
    ScopedProfiler(const std::string& name) : name_(name) {
        HighResProfiler::get_instance().begin_sample(name_);
    }
    
    ~ScopedProfiler() {
        HighResProfiler::get_instance().end_sample();
    }
    
private:
    std::string name_;
};

// Convenience macros
#define PROFILE_FUNCTION() ScopedProfiler profiler_##__LINE__(__FUNCTION__)
#define PROFILE_SCOPE(name) ScopedProfiler profiler_##__LINE__(name)
#define PROFILE_THREAD(name) HighResProfiler::get_instance().begin_sample(name)

} // namespace performance
} // namespace streaming
// include/streaming/container/fragmentation.hpp
#pragma once

#include "media_container.hpp"
#include <vector>
#include <queue>

namespace streaming {
namespace container {

class FragmentManager {
public:
    struct Fragment {
        uint64_t start_time;
        uint64_t duration;
        uint64_t file_offset;
        uint32_t size;
        std::vector<uint32_t> track_ids;
        bool completed;
    };

    FragmentManager();
    
    bool initialize(const ContainerConfig& config);
    void set_fragment_duration(uint32_t duration_ms);
    void set_max_fragment_size(uint32_t max_size);
    
    // Fragment management
    bool should_create_fragment(uint64_t timestamp) const;
    Fragment* create_fragment(uint64_t timestamp);
    bool finalize_fragment(Fragment* fragment);
    
    // Fragment indexing
    void add_fragment_index(const Fragment& fragment);
    std::vector<Fragment> get_fragments_in_range(uint64_t start_time, uint64_t end_time) const;
    
    // Live streaming support
    void enable_live_mode(bool enable);
    void set_live_window_duration(uint32_t duration_ms);

private:
    bool check_fragment_conditions(uint64_t timestamp) const;
    void cleanup_old_fragments(); // For live streaming

private:
    ContainerConfig config_;
    std::vector<Fragment> fragments_;
    Fragment* current_fragment_ = nullptr;
    
    // Live streaming state
    bool live_mode_ = false;
    uint32_t live_window_ms_ = 30000; // 30 second window
};

} // namespace container
} // namespace streaming
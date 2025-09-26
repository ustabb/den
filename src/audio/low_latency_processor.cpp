// src/audio/low_latency_processor.cpp
#include "streaming/audio/audio_processor.hpp"

namespace streaming {
namespace audio {

class LowLatencyAudioProcessor {
public:
    struct AudioLatencyConfig {
        uint32_t buffer_size_ms = 5;        // 5ms buffers
        uint32_t processing_time_ms = 2;    // 2ms max processing
        bool enable_short_frames = true;    // Use smaller frames
    };

    bool initialize(const AudioLatencyConfig& config) {
        config_ = config;
        
        // Optimize for low latency
        set_short_frame_sizes();
        disable_high_latency_effects();
        
        std::cout << "🎵 LowLatencyAudioProcessor: " << config_.buffer_size_ms 
                  << "ms buffers, " << config_.processing_time_ms << "ms processing" << std::endl;
        
        return true;
    }
    
    bool process_audio_low_latency(const AudioFrame& input, AudioFrame& output) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Minimal processing for low latency
        apply_essential_processing_only(input, output);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(
            end - start).count() / 1000.0;
            
        if (processing_time > config_.processing_time_ms) {
            std::cout << "⚠️ Audio processing slow: " << processing_time << "ms" << std::endl;
            simplify_processing_pipeline();
        }
        
        return true;
    }

private:
    void set_short_frame_sizes() {
        // Use very short frames for low latency
        // Opus supports frames as short as 2.5ms
    }
    
    void disable_high_latency_effects() {
        // Disable effects that add latency
        // - Long FIR filters
        // - Complex reverb
        // - Look-ahead limiters
    }
};

} // namespace audio
} // namespace streaming
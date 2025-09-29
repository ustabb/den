// include/streaming/audio/audio_processor.hpp
#pragma once

#include "audio_codec.hpp"
#include "resampler.hpp"
#include <memory>
#include <vector>

namespace streaming {
namespace audio {

class AudioProcessor {
public:
    AudioProcessor();
    ~AudioProcessor();
    
    bool initialize(const AudioConfig& input_config, const AudioConfig& output_config);
    void process_audio(const AudioFrame& input, AudioFrame& output);
    
    // Audio processing chain
    void apply_noise_reduction(AudioFrame& frame);
    void apply_automatic_gain_control(AudioFrame& frame);
    void apply_echo_cancellation(AudioFrame& frame);
    void apply_voice_enhancement(AudioFrame& frame);
    
    // Streaming optimizations
    void set_target_latency_ms(uint32_t latency_ms);
    void enable_adaptive_bitrate(bool enable);
    void adjust_quality_based_on_network(float packet_loss, uint32_t available_bandwidth);

private:
    void resample_audio(const AudioFrame& input, AudioFrame& output);
    void normalize_audio_level(AudioFrame& frame);
    void detect_clipping(const AudioFrame& frame);
    
private:
    AudioConfig input_config_;
    AudioConfig output_config_;
    std::unique_ptr<Resampler> resampler_;
    
    // Processing state
    double rms_level_ = 0.0;
    double peak_level_ = 0.0;
    uint32_t clipping_count_ = 0;
    
    // Adaptive streaming
    bool adaptive_bitrate_enabled_ = true;
    uint32_t target_latency_ms_ = 60; // Low latency for streaming
    float current_quality_factor_ = 1.0f;
};

} // namespace audio
} // namespace streaming
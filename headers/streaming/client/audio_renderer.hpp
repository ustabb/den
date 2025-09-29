// include/streaming/client/audio_renderer.hpp
#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <SDL2/SDL_audio.h>

namespace streaming {
namespace client {

class AudioRenderer {
public:
    struct AudioConfig {
        uint32_t sample_rate = 48000;
        uint16_t channels = 2;
        uint16_t sample_format = AUDIO_S16SYS; // Signed 16-bit
        uint32_t buffer_size = 1024;
        uint32_t num_buffers = 3;
        bool low_latency = true;
        float volume = 1.0f;
    };

    struct AudioFrame {
        std::vector<int16_t> samples;
        uint32_t sample_rate;
        uint16_t channels;
        uint64_t timestamp;
        uint32_t sample_count;
    };

    AudioRenderer();
    ~AudioRenderer();
    
    bool initialize(const AudioConfig& config);
    void shutdown();
    
    void submit_frame(const AudioFrame& frame);
    void play();
    void pause();
    void set_volume(float volume);
    void set_playback_rate(double rate);
    
    // Audio processing
    void resample_audio(const AudioFrame& input, AudioFrame& output, uint32_t target_rate);
    void apply_volume(AudioFrame& frame, float volume);
    void sync_with_video(int64_t audio_video_delta);
    
    // Latency control
    uint32_t get_current_latency_ms() const;
    void set_max_latency_ms(uint32_t latency_ms);
    void enable_audio_sync(bool enable);

private:
    static void audio_callback(void* userdata, Uint8* stream, int len);
    void audio_mixing_callback(Uint8* output_stream, int output_len);
    
    void resample_frame(const AudioFrame& input, AudioFrame& output);
    void handle_buffer_underrun();
    void handle_buffer_overrun();
    
    AudioConfig config_;
    SDL_AudioDeviceID audio_device_ = 0;
    
    // Audio buffer management
    std::vector<AudioFrame> audio_queue_;
    mutable std::mutex audio_mutex_;
    std::condition_variable audio_available_;
    
    // Resampling state
    std::unique_ptr<Resampler> resampler_;
    uint32_t output_sample_rate_;
    
    // Sync state
    int64_t audio_video_offset_ = 0;
    bool audio_sync_enabled_ = true;
    uint32_t max_latency_ms_ = 100;
};

} // namespace client
} // namespace streaming
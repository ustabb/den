// include/streaming/client/streaming_client.hpp
#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <functional>
#include <SDL2/SDL.h>

namespace streaming {
namespace client {

class StreamingClient {
public:
    struct ClientConfig {
        // Video settings
        uint32_t video_width = 1920;
        uint32_t video_height = 1080;
        uint32_t target_fps = 60;
        bool hardware_acceleration = true;
        
        // Audio settings
        uint32_t audio_sample_rate = 48000;
        uint16_t audio_channels = 2;
        uint32_t audio_buffer_size = 1024;
        
        // Network settings
        uint32_t buffer_duration_ms = 3000; // 3 second buffer
        uint32_t max_reorder_delay_ms = 100;
        bool low_latency_mode = true;
        
        // Rendering settings
        bool vsync_enabled = true;
        uint32_t renderer_type = 0; // 0=Software, 1=OpenGL, 2=Vulkan
    };

    struct PlayerState {
        enum State {
            STOPPED,
            CONNECTING,
            BUFFERING,
            PLAYING,
            PAUSED,
            ERROR
        };
        
        State current_state = STOPPED;
        double current_time = 0.0;
        double duration = 0.0;
        double playback_rate = 1.0;
        float volume = 1.0f;
        bool muted = false;
        uint64_t frames_decoded = 0;
        uint64_t frames_dropped = 0;
        uint32_t current_bitrate = 0;
        uint32_t network_latency = 0;
        float packet_loss = 0.0f;
    };

    // Event callbacks
    using StateChangedCallback = std::function<void(PlayerState::State old_state, PlayerState::State new_state)>;
    using ErrorCallback = std::function<void(const std::string& error_message)>;
    using StatisticsCallback = std::function<void(const PlayerState& stats)>;

    StreamingClient();
    ~StreamingClient();
    
    bool initialize(const ClientConfig& config);
    void shutdown();
    
    // Playback control
    bool play(const std::string& url);
    void pause();
    void resume();
    void stop();
    void seek(double timestamp_seconds);
    void set_volume(float volume);
    void set_playback_rate(double rate);
    
    // Network management
    void set_network_bandwidth(uint32_t bandwidth_bps);
    void set_max_latency(uint32_t latency_ms);
    void enable_adaptive_bitrate(bool enable);
    
    // Event registration
    void set_state_changed_callback(StateChangedCallback callback);
    void set_error_callback(ErrorCallback callback);
    void set_statistics_callback(StatisticsCallback callback);

    PlayerState get_current_state() const;

private:
    void main_loop();
    void network_loop();
    void video_loop();
    void audio_loop();
    void control_loop();
    
    // Stream handling
    bool connect_to_stream(const std::string& url);
    void disconnect();
    void handle_stream_data(const uint8_t* data, size_t size);
    
    // Buffer management
    void manage_playback_buffers();
    void adjust_buffering_strategy();
    
    ClientConfig config_;
    PlayerState state_;
    
    // Threading
    std::atomic<bool> running_{false};
    std::thread main_thread_;
    std::thread network_thread_;
    std::thread video_thread_;
    std::thread audio_thread_;
    std::thread control_thread_;
    
    // Components
    std::unique_ptr<VideoRenderer> video_renderer_;
    std::unique_ptr<AudioRenderer> audio_renderer_;
    std::unique_ptr<Demuxer> demuxer_;
    std::unique_ptr<Decoder> decoder_;
    std::unique_ptr<NetworkClient> network_client_;
    
    // Callbacks
    StateChangedCallback state_changed_cb_;
    ErrorCallback error_cb_;
    StatisticsCallback statistics_cb_;
    
    // SDL for window management
    SDL_Window* window_ = nullptr;
    SDL_Renderer* sdl_renderer_ = nullptr;
};

} // namespace client
} // namespace streaming
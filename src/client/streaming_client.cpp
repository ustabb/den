// src/client/streaming_client.cpp
#include "streaming/client/streaming_client.hpp"
#include "streaming/client/video_renderer.hpp"
#include "streaming/client/audio_renderer.hpp"
#include "streaming/client/demuxer.hpp"
#include "streaming/client/decoder.hpp"
#include "streaming/client/network_client.hpp"
#include "streaming/utils/logger.hpp"
#include <SDL2/SDL.h>
#include <thread>

namespace streaming {
namespace client {

StreamingClient::StreamingClient() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        LOG_ERROR("SDL initialization failed: {}", SDL_GetError());
        return;
    }
    
    video_renderer_ = std::make_unique<VideoRenderer>();
    audio_renderer_ = std::make_unique<AudioRenderer>();
    demuxer_ = std::make_unique<Demuxer>();
    decoder_ = std::make_unique<Decoder>();
    network_client_ = std::make_unique<NetworkClient>();
}

bool StreamingClient::initialize(const ClientConfig& config) {
    config_ = config;
    
    // Create SDL window
    window_ = SDL_CreateWindow("Streaming Player",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              config.video_width, config.video_height,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    
    if (!window_) {
        LOG_ERROR("Failed to create SDL window: {}", SDL_GetError());
        return false;
    }
    
    // Initialize video renderer
    VideoRenderer::RenderConfig render_config;
    render_config.output_width = config.video_width;
    render_config.output_height = config.video_height;
    render_config.use_opengl = config.hardware_acceleration;
    render_config.vsync_enabled = config.vsync_enabled;
    
    if (!video_renderer_->initialize(render_config, window_)) {
        LOG_ERROR("Failed to initialize video renderer");
        return false;
    }
    
    // Initialize audio renderer
    AudioRenderer::AudioConfig audio_config;
    audio_config.sample_rate = config.audio_sample_rate;
    audio_config.channels = config.audio_channels;
    audio_config.buffer_size = config.audio_buffer_size;
    audio_config.low_latency = config.low_latency_mode;
    
    if (!audio_renderer_->initialize(audio_config)) {
        LOG_ERROR("Failed to initialize audio renderer");
        return false;
    }
    
    // Initialize decoder
    Decoder::DecoderConfig decoder_config;
    decoder_config.hardware_acceleration = config.hardware_acceleration;
    decoder_config.max_decode_time_ms = 1000 / config.target_fps;
    
    if (!decoder_->initialize(decoder_config)) {
        LOG_ERROR("Failed to initialize decoder");
        return false;
    }
    
    LOG_INFO("StreamingClient initialized successfully");
    return true;
}

bool StreamingClient::play(const std::string& url) {
    if (state_.current_state != PlayerState::STOPPED) {
        stop();
    }
    
    state_.current_state = PlayerState::CONNECTING;
    if (state_changed_cb_) {
        state_changed_cb_(PlayerState::STOPPED, PlayerState::CONNECTING);
    }
    
    // Start main loop
    running_.store(true, std::memory_order_release);
    main_thread_ = std::thread([this, url]() { main_loop(); });
    
    // Start worker threads
    network_thread_ = std::thread([this, url]() { network_loop(); });
    video_thread_ = std::thread([this]() { video_loop(); });
    audio_thread_ = std::thread([this]() { audio_loop(); });
    control_thread_ = std::thread([this]() { control_loop(); });
    
    LOG_INFO("Starting playback: {}", url);
    return true;
}

void StreamingClient::main_loop() {
    LOG_INFO("Main loop started");
    
    SDL_Event event;
    auto last_statistics_time = std::chrono::steady_clock::now();
    
    while (running_.load(std::memory_order_acquire)) {
        // Handle SDL events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    stop();
                    break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        // Handle window resize
                        video_renderer_->set_display_fps(config_.target_fps);
                    }
                    break;
                case SDL_KEYDOWN:
                    // Handle keyboard controls
                    if (event.key.keysym.sym == SDLK_SPACE) {
                        if (state_.current_state == PlayerState::PLAYING) {
                            pause();
                        } else {
                            resume();
                        }
                    }
                    break;
            }
        }
        
        // Update statistics periodically
        auto current_time = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(
            current_time - last_statistics_time).count() >= 1) {
            
            if (statistics_cb_) {
                statistics_cb_(state_);
            }
            last_statistics_time = current_time;
        }
        
        // Small delay to prevent busy waiting
        SDL_Delay(10);
    }
    
    LOG_INFO("Main loop stopped");
}

void StreamingClient::network_loop() {
    LOG_INFO("Network loop started");
    
    NetworkClient::NetworkConfig net_config;
    net_config.server_url = "http://localhost:8080/stream.flv"; // Example URL
    net_config.connection_timeout_ms = 5000;
    
    if (!network_client_->initialize(net_config)) {
        LOG_ERROR("Failed to initialize network client");
        return;
    }
    
    if (!network_client_->connect()) {
        LOG_ERROR("Failed to connect to stream");
        return;
    }
    
    state_.current_state = PlayerState::BUFFERING;
    if (state_changed_cb_) {
        state_changed_cb_(PlayerState::CONNECTING, PlayerState::BUFFERING);
    }
    
    std::vector<uint8_t> buffer(65536);
    
    while (running_.load(std::memory_order_acquire)) {
        auto data = network_client_->download_data(4096);
        if (!data.empty()) {
            handle_stream_data(data.data(), data.size());
        } else {
            // Handle network error or end of stream
            break;
        }
        
        // Simulate network timing
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    network_client_->disconnect();
    LOG_INFO("Network loop stopped");
}

void StreamingClient::video_loop() {
    LOG_INFO("Video loop started");
    
    auto last_frame_time = std::chrono::steady_clock::now();
    const auto frame_duration = std::chrono::microseconds(1000000 / config_.target_fps);
    
    while (running_.load(std::chrono::memory_order_acquire)) {
        if (state_.current_state != PlayerState::PLAYING) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        // Decode and render video frame
        // This would involve:
        // 1. Getting encoded data from demuxer
        // 2. Decoding with hardware acceleration
        // 3. Rendering to screen
        
        video_renderer_->render_frame();
        video_renderer_->present();
        
        state_.frames_decoded++;
        state_.current_time = static_cast<double>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - last_frame_time).count()) / 1000.0;
        
        // Frame rate control
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = current_time - last_frame_time;
        if (elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - elapsed);
        }
        last_frame_time = std::chrono::steady_clock::now();
    }
    
    LOG_INFO("Video loop stopped");
}

void StreamingClient::shutdown() {
    stop();
    
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    
    SDL_Quit();
    LOG_INFO("StreamingClient shutdown complete");
}

} // namespace client
} // namespace streaming
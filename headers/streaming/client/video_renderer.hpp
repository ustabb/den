// include/streaming/client/video_renderer.hpp
#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <SDL2/SDL.h>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

namespace streaming {
namespace client {

class VideoRenderer {
public:
    struct VideoFrame {
        std::vector<uint8_t> y_plane;
        std::vector<uint8_t> u_plane;
        std::vector<uint8_t> v_plane;
        uint32_t width;
        uint32_t height;
        uint64_t timestamp;
        bool is_keyframe;
        uint32_t stride_y;
        uint32_t stride_uv;
    };

    struct RenderConfig {
        uint32_t output_width;
        uint32_t output_height;
        uint32_t max_queue_size = 3; // Triple buffering
        bool use_opengl = true;
        bool use_shaders = true;
        bool vsync_enabled = true;
    };

    VideoRenderer();
    ~VideoRenderer();
    
    bool initialize(const RenderConfig& config, SDL_Window* window);
    void shutdown();
    
    void submit_frame(const VideoFrame& frame);
    void render_frame();
    void present();
    
    // Performance optimization
    void set_display_fps(uint32_t fps);
    void enable_frame_dropping(bool enable);
    void set_scaling_quality(uint32_t quality); // 0=nearest, 1=linear, 2=best
    
    // Statistics
    uint32_t get_rendered_frames() const { return frames_rendered_; }
    uint32_t get_dropped_frames() const { return frames_dropped_; }
    double get_render_time_ms() const { return avg_render_time_; }

private:
    // Rendering backends
    bool initialize_opengl();
    bool initialize_software();
    void render_opengl();
    void render_software();
    
    // Shader management
    bool compile_shaders();
    void create_textures();
    void update_textures(const VideoFrame& frame);
    
    // Frame queue management
    bool should_drop_frame(const VideoFrame& frame) const;
    VideoFrame get_next_frame_for_rendering();

    RenderConfig config_;
    SDL_Window* window_ = nullptr;
    SDL_Renderer* sdl_renderer_ = nullptr;
    SDL_Texture* sdl_texture_ = nullptr;
    
    // OpenGL resources
    GLuint program_id_ = 0;
    GLuint y_texture_ = 0, u_texture_ = 0, v_texture_ = 0;
    GLuint vao_ = 0, vbo_ = 0;
    
    // Frame management
    std::vector<VideoFrame> frame_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable frame_available_;
    
    // Performance tracking
    std::atomic<uint32_t> frames_rendered_{0};
    std::atomic<uint32_t> frames_dropped_{0};
    std::atomic<double> avg_render_time_{0};
};

} // namespace client
} // namespace streaming
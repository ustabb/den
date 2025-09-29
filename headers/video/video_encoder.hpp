// include/streaming/video/video_encoder.hpp
#pragma once

// FFmpeg wrapper removed
#include "../processing/bbr_turbo.hpp"
#include <memory>

namespace streaming {
namespace video {

class VideoEncoder {
public:
    VideoEncoder();
    ~VideoEncoder();
    
    bool initialize(const StreamConfig& config);
    void set_congestion_controller(std::shared_ptr<BBRTurbo> bbr);
    
    // Adaptive bitrate encoding
    bool encode_frame(AVFrame* frame);
    AVPacket* get_encoded_packet();
    
    // Streaming optimizations
    void adjust_bitrate(int target_bitrate);
    void set_gop_size(int gop_size);
    void enable_low_delay(bool enable);
    
    // Quality control
    void set_quality_preset(const std::string& preset);
    void set_speed_preset(const std::string& preset);

private:
    bool init_encoder(const StreamConfig& config);
    bool init_h264_encoder();
    bool init_h265_encoder();
    void apply_low_latency_settings();
    
private:
    // AVCodecContext* encoder_ctx_ = nullptr; // FFmpeg dependency removed
    std::shared_ptr<BBRTurbo> congestion_controller_;
    
    AVPacket* output_packet_ = nullptr;
    StreamConfig config_;
    
    int current_bitrate_ = 1000000; // 1 Mbps
    int target_bitrate_ = 1000000;
    bool low_delay_mode_ = true;
};

} // namespace video
} // namespace streaming
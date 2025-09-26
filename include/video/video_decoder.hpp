// include/streaming/video/video_decoder.hpp
#pragma once

#include "ffmpeg_wrapper.hpp"
#include "../network/zero_copy_buffer.hpp"
#include <atomic>
#include <queue>

namespace streaming {
namespace video {

class VideoDecoder {
public:
    VideoDecoder();
    ~VideoDecoder();
    
    bool initialize(const StreamConfig& config);
    void set_output_buffer(std::shared_ptr<ZeroCopyBuffer> buffer);
    
    // Streaming-optimized decoding
    bool decode_packet(const uint8_t* data, size_t size);
    AVFrame* get_decoded_frame();
    
    // Low-latency optimizations
    void enable_frame_dropping(bool enable);
    void set_max_decode_time_ms(int ms);
    
    // Performance monitoring
    double get_current_decode_time() const;
    size_t get_frames_decoded() const;

private:
    bool init_decoder(const StreamConfig& config);
    AVFrame* decode_internal(AVPacket* packet);
    bool should_drop_frame(int64_t pts);
    
private:
    AVCodecContext* decoder_ctx_ = nullptr;
    std::shared_ptr<ZeroCopyBuffer> output_buffer_;
    
    std::queue<AVFrame*> frame_queue_;
    std::mutex queue_mutex_;
    
    std::atomic<size_t> frames_decoded_{0};
    std::atomic<double> total_decode_time_{0};
    
    bool frame_dropping_enabled_ = true;
    int max_decode_time_ms_ = 33; // 30fps için 33ms
    int64_t last_pts_ = 0;
};

} // namespace video
} // namespace streaming
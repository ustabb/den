// include/streaming/video/frame_processor.hpp
#pragma once

#include "video_decoder.hpp"
#include "video_encoder.hpp"
#include "../processing/stream_batcher.hpp"
#include <atomic>

namespace streaming {
namespace video {

class FrameProcessor {
public:
    FrameProcessor();
    ~FrameProcessor();
    
    bool initialize(const StreamConfig& config);
    void start_processing();
    void stop_processing();
    
    // Frame processing pipeline
    bool process_frame(const uint8_t* data, size_t size);
    bool process_frame_zerocopy(AVFrame* frame);
    
    // Streaming optimizations
    void set_output_callback(std::function<void(const uint8_t*, size_t)> callback);
    void enable_adaptive_quality(bool enable);

private:
    void processing_loop();
    bool process_single_frame(AVFrame* frame);
    void apply_streaming_filters(AVFrame* frame);
    
private:
    std::unique_ptr<VideoDecoder> decoder_;
    std::unique_ptr<VideoEncoder> encoder_;
    std::shared_ptr<StreamBatcher> packet_batcher_;
    
    std::atomic<bool> running_{false};
    std::thread processing_thread_;
    
    std::function<void(const uint8_t*, size_t)> output_callback_;
    StreamConfig config_;
    
    // Performance tracking
    std::atomic<uint64_t> frames_processed_{0};
    std::atomic<double> avg_processing_time_{0};
};

} // namespace video
} // namespace streaming
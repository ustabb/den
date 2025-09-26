// include/streaming/engine/streaming_pipeline.hpp
#pragma once

#include "types.hpp"
#include "../video/frame_processor.hpp"
#include "../network/socket_manager.hpp"
#include <atomic>
#include <thread>
#include <functional>

namespace streaming {

class StreamingPipeline {
public:
    StreamingPipeline();
    ~StreamingPipeline();
    
    bool initialize(const StreamConfig& config);
    void start_streaming();
    void stop_streaming();
    
    // Video input interface
    bool submit_video_frame(const VideoFrame& frame);
    bool submit_encoded_data(const uint8_t* data, size_t size);
    
    // Callbacks
    void set_error_callback(std::function<void(const std::string&)> callback);
    void set_status_callback(std::function<void(const std::string&)> callback);

private:
    void streaming_loop();
    void network_loop();
    bool process_and_stream(const VideoFrame& frame);
    
private:
    StreamConfig config_;
    std::unique_ptr<video::FrameProcessor> frame_processor_;
    network::SocketManager& socket_manager_;
    
    std::atomic<bool> running_{false};
    std::thread streaming_thread_;
    std::thread network_thread_;
    
    std::function<void(const std::string&)> error_callback_;
    std::function<void(const std::string&)> status_callback_;
    
    // Performance metrics
    std::atomic<uint64_t> frames_processed_{0};
    std::atomic<uint64_t> bytes_sent_{0};
};

} // namespace streaming
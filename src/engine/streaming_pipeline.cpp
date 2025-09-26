// src/engine/streaming_pipeline.cpp
#include "streaming/engine/streaming_pipeline.hpp"
#include "streaming/utils/logger.hpp"

namespace streaming {

StreamingPipeline::StreamingPipeline() 
    : socket_manager_(network::SocketManager::get_instance()) {
    
    frame_processor_ = std::make_unique<video::FrameProcessor>();
}

bool StreamingPipeline::initialize(const StreamConfig& config) {
    config_ = config;
    
    // Initialize socket manager first
    if (!socket_manager_.initialize(config)) {
        LOG_ERROR("SocketManager initialization failed");
        return false;
    }
    
    // Initialize video processing pipeline
    if (!frame_processor_->initialize(config)) {
        LOG_ERROR("FrameProcessor initialization failed");
        return false;
    }
    
    LOG_INFO("StreamingPipeline initialized for {}:{}", config.host, config.port);
    return true;
}

void StreamingPipeline::start_streaming() {
    if (running_.exchange(true, std::memory_order_acq_rel)) {
        LOG_WARN("Streaming already started");
        return;
    }
    
    streaming_thread_ = std::thread([this]() { streaming_loop(); });
    network_thread_ = std::thread([this]() { network_loop(); });
    
    if (status_callback_) {
        status_callback_("Streaming started");
    }
    
    LOG_INFO("Streaming started to {}:{}", config_.host, config_.port);
}

void StreamingPipeline::streaming_loop() {
    while (running_.load(std::memory_order_acquire)) {
        // Burada frame processing ve encoding işlemleri yapılacak
        // Şimdilik simüle edelim
        
        // Simulate frame processing
        VideoFrame frame;
        frame.width = 1920;
        frame.height = 1080;
        frame.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        if (process_and_stream(frame)) {
            frames_processed_.fetch_add(1, std::memory_order_relaxed);
        }
        
        // Control streaming rate
        std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30fps
    }
}

bool StreamingPipeline::process_and_stream(const VideoFrame& frame) {
    try {
        // Process frame through video pipeline
        std::vector<uint8_t> encoded_data;
        // frame_processor_->process_frame(frame, encoded_data); // TODO: Implement
        
        // Simulate encoded data
        encoded_data.resize(1024, 0xAA); // 1KB test data
        
        // Stream via socket manager
        if (socket_manager_.stream_video_data(config_.host, config_.port, 
                                            encoded_data.data(), encoded_data.size())) {
            bytes_sent_.fetch_add(encoded_data.size(), std::memory_order_relaxed);
            return true;
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Frame processing failed: {}", e.what());
        if (error_callback_) {
            error_callback_(e.what());
        }
    }
    
    return false;
}

void StreamingPipeline::stop_streaming() {
    running_.store(false, std::memory_order_release);
    
    if (streaming_thread_.joinable()) {
        streaming_thread_.join();
    }
    if (network_thread_.joinable()) {
        network_thread_.join();
    }
    
    if (status_callback_) {
        status_callback_("Streaming stopped");
    }
    
    LOG_INFO("Streaming stopped. Processed {} frames, sent {} bytes", 
             frames_processed_.load(), bytes_sent_.load());
}

} // namespace streaming
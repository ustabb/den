// examples/latency_measurement_test.cpp
#include "streaming/engine/latency_analyzer.hpp"
#include "streaming/video/low_latency_encoder.hpp"
#include "streaming/audio/low_latency_processor.hpp"
#include <chrono>
#include <iostream>

using namespace streaming;

class EndToEndLatencyMeasurer {
private:
    video::LowLatencyEncoder video_encoder_;
    audio::LowLatencyAudioProcessor audio_processor_;
    LatencyAnalyzer latency_analyzer_;
    
    uint64_t frame_id_ = 0;
    
public:
    bool initialize() {
        // Video encoder setup
        video::LowLatencyEncoder::LowLatencyConfig video_config;
        video_config.max_encoding_time_ms = 10;  // 10ms max encoding
        video_config.target_frame_size_ms = 8;   // 125 FPS target
        video_config.enable_frame_dropping = true;
        
        if (!video_encoder_.initialize(video_config)) {
            return false;
        }
        
        // Audio processor setup
        audio::LowLatencyAudioProcessor::AudioLatencyConfig audio_config;
        audio_config.buffer_size_ms = 5;
        audio_config.processing_time_ms = 2;
        
        if (!audio_processor_.initialize(audio_config)) {
            return false;
        }
        
        std::cout << "🚀 End-to-End Latency Measurement Initialized" << std::endl;
        return true;
    }
    
    void process_frame() {
        auto start_time = std::chrono::high_resolution_clock::now();
        latency_analyzer_.mark_stage("frame_capture", frame_id_);
        
        // Simulate frame processing pipeline
        process_video_pipeline();
        process_audio_pipeline();
        process_network_transmission();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto total_latency = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time).count() / 1000.0;
        
        std::cout << "Frame " << frame_id_ << " - Total latency: " 
                  << total_latency << "ms" << std::endl;
        
        frame_id_++;
        
        // Real-time latency monitoring
        if (total_latency > 33.0) { // 30 FPS threshold
            std::cout << "⚠️ High latency detected: " << total_latency << "ms" << std::endl;
            trigger_optimizations();
        }
    }
    
    void generate_report() {
        latency_analyzer_.generate_latency_report();
    }

private:
    void process_video_pipeline() {
        latency_analyzer_.mark_stage("video_start", frame_id_);
        
        VideoFrame frame;
        std::vector<uint8_t> encoded;
        bool dropped = false;
        
        video_encoder_.encode_frame_low_latency(frame, encoded, dropped);
        
        if (!dropped) {
            latency_analyzer_.mark_stage("video_encoded", frame_id_);
        }
    }
    
    void process_audio_pipeline() {
        latency_analyzer_.mark_stage("audio_start", frame_id_);
        
        AudioFrame audio_in, audio_out;
        audio_processor_.process_audio_low_latency(audio_in, audio_out);
        
        latency_analyzer_.mark_stage("audio_processed", frame_id_);
    }
    
    void process_network_transmission() {
        latency_analyzer_.mark_stage("network_start", frame_id_);
        
        // Simulate network transmission
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        
        latency_analyzer_.mark_stage("network_sent", frame_id_);
    }
    
    void trigger_optimizations() {
        std::cout << "🔧 Triggering latency optimizations..." << std::endl;
        // Reduce video quality
        // Increase frame dropping
        // Simplify audio processing
    }
};

int main() {
    try {
        EndToEndLatencyMeasurer measurer;
        if (!measurer.initialize()) {
            std::cerr << "Failed to initialize latency measurer" << std::endl;
            return 1;
        }
        
        std::cout << "🎯 Starting latency measurement (100 frames)..." << std::endl;
        std::cout << "Target: < 33ms for 30 FPS streaming" << std::endl;
        std::cout << "=============================================" << std::endl;
        
        // Measure latency for 100 frames
        for (int i = 0; i < 100; ++i) {
            measurer.process_frame();
            std::this_thread::sleep_for(std::chrono::milliseconds(33)); // 30 FPS
        }
        
        std::cout << "\n📊 Latency Measurement Complete" << std::endl;
        std::cout << "=============================================" << std::endl;
        measurer.generate_report();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
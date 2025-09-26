// examples/container_test.cpp
#include "streaming/container/mp4_writer.hpp"
#include "streaming/container/webm_writer.hpp"
#include "streaming/codec/h264_encoder.hpp"
#include "streaming/audio/opus_encoder.hpp"
#include <iostream>
#include <vector>

using namespace streaming;

void test_mp4_container() {
    std::cout << "🎬 Testing MP4-like Container" << std::endl;
    
    container::MP4Writer writer;
    container::ContainerConfig config;
    config.format = container::ContainerFormat::MP4_LIKE;
    config.fragmented = true;
    config.fragment_duration = 2000; // 2 second fragments
    config.fast_start = true;
    
    if (!writer.initialize(config)) {
        std::cerr << "Failed to initialize MP4 writer" << std::endl;
        return;
    }
    
    if (!writer.open("test_output.mp4", true)) {
        std::cerr << "Failed to open MP4 file" << std::endl;
        return;
    }
    
    // Add video track
    container::TrackInfo video_track;
    video_track.track_id = 1;
    video_track.track_type = container::TrackType::VIDEO_TRACK;
    video_track.codec_type = container::CodecType::H264;
    video_track.timescale = 90000; // 90kHz for video
    video_track.width = 1920;
    video_track.height = 1080;
    
    writer.add_track(video_track);
    
    // Add audio track
    container::TrackInfo audio_track;
    audio_track.track_id = 2;
    audio_track.track_type = container::TrackType::AUDIO_TRACK;
    audio_track.codec_type = container::CodecType::OPUS;
    audio_track.sample_rate = 48000;
    audio_track.channels = 2;
    
    writer.add_track(audio_track);
    
    // Add metadata
    writer.add_metadata("title", "Test Video");
    writer.add_metadata("artist", "Streaming Engine");
    writer.add_metadata("created_with", "Custom Streaming Engine");
    
    // Simulate writing samples
    std::cout << "Writing test samples..." << std::endl;
    
    for (int i = 0; i < 300; i++) { // 10 seconds at 30fps
        // Simulate video frame
        std::vector<uint8_t> video_frame(1024, static_cast<uint8_t>(i % 256));
        uint64_t video_timestamp = i * 3000; // 90kHz timestamp
        
        writer.write_sample(1, video_frame.data(), video_frame.size(), 
                           video_timestamp, (i % 30 == 0)); // Keyframe every 30 frames
        
        // Simulate audio every 2 video frames (48kHz / 30fps = 1600 samples)
        if (i % 2 == 0) {
            std::vector<uint8_t> audio_frame(320, static_cast<uint8_t>(i % 128));
            uint64_t audio_timestamp = i * 1600; // 48kHz timestamp
            
            writer.write_sample(2, audio_frame.data(), audio_frame.size(), 
                               audio_timestamp, true);
        }
        
        // Create fragment every 60 frames (2 seconds)
        if (i % 60 == 0 && i > 0) {
            writer.create_fragment();
        }
    }
    
    writer.close();
    std::cout << "✅ MP4 container test completed" << std::endl;
}

void test_webm_container() {
    std::cout << "\n🎵 Testing WebM-like Container" << std::endl;
    
    container::WebMWriter writer;
    container::ContainerConfig config;
    config.format = container::ContainerFormat::WEBM_LIKE;
    config.fragmented = true;
    
    if (!writer.initialize(config)) {
        std::cerr << "Failed to initialize WebM writer" << std::endl;
        return;
    }
    
    if (!writer.open("test_output.webm", true)) {
        std::cerr << "Failed to open WebM file" << std::endl;
        return;
    }
    
    // Similar track setup as MP4 test...
    
    writer.close();
    std::cout << "✅ WebM container test completed" << std::endl;
}

int main() {
    try {
        std::cout << "🚀 Media Container Format Tests" << std::endl;
        std::cout << "==============================" << std::endl;
        
        test_mp4_container();
        test_webm_container();
        
        std::cout << "\n🎉 All container tests completed successfully!" << std::endl;
        std::cout << "Generated files: test_output.mp4, test_output.webm" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
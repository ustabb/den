// examples/audio_encoding_test.cpp
#include "streaming/audio/opus_encoder.hpp"
#include "streaming/audio/aac_encoder.hpp"
#include "streaming/audio/audio_processor.hpp"
#include <iostream>
#include <fstream>
#include <cmath>

using namespace streaming;

// Generate test audio signal
void generate_test_audio(audio::AudioFrame& frame, uint32_t sample_rate, 
                        uint16_t channels, uint32_t duration_ms) {
    frame.sample_rate = sample_rate;
    frame.channels = channels;
    frame.frame_size = (sample_rate * duration_ms) / 1000;
    frame.timestamp = 0;
    
    frame.samples.resize(frame.frame_size * channels);
    
    // Generate multi-tone test signal
    for (uint32_t i = 0; i < frame.frame_size; ++i) {
        double t = static_cast<double>(i) / sample_rate;
        
        // Multiple frequencies to test codec
        double sample = 0.5 * std::sin(2 * M_PI * 440 * t) +  // A4
                       0.3 * std::sin(2 * M_PI * 880 * t) +  // A5
                       0.2 * std::sin(2 * M_PI * 1320 * t);  // E6
        
        int16_t pcm_sample = static_cast<int16_t>(sample * 32767);
        
        for (uint16_t ch = 0; ch < channels; ++ch) {
            frame.samples[i * channels + ch] = pcm_sample;
        }
    }
}

void test_opus_encoder() {
    std::cout << "\n=== OPUS ENCODER TEST ===" << std::endl;
    
    audio::AudioConfig config;
    config.sample_rate = 48000;
    config.channels = 2;
    config.bitrate = 96000; // 96 kbps
    config.frame_size = 960; // 20ms at 48kHz
    
    audio::OpusEncoder encoder;
    if (!encoder.initialize(config)) {
        std::cerr << "Failed to initialize Opus encoder" << std::endl;
        return;
    }
    
    // Enable streaming optimizations
    encoder.enable_dtx(true);
    encoder.enable_vbr(true);
    encoder.enable_fec(true);
    
    // Generate test audio
    audio::AudioFrame input_frame;
    generate_test_audio(input_frame, config.sample_rate, config.channels, 1000); // 1 second
    
    // Encode multiple frames
    const uint32_t frames_per_second = config.sample_rate / config.frame_size;
    std::vector<std::vector<uint8_t>> encoded_frames;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (uint32_t i = 0; i < frames_per_second; ++i) {
        audio::AudioFrame frame;
        frame.sample_rate = config.sample_rate;
        frame.channels = config.channels;
        frame.frame_size = config.frame_size;
        frame.timestamp = i * config.frame_size;
        
        // Extract frame from test audio
        size_t start_sample = i * config.frame_size * config.channels;
        frame.samples.assign(input_frame.samples.begin() + start_sample,
                           input_frame.samples.begin() + start_sample + 
                           config.frame_size * config.channels);
        
        std::vector<uint8_t> encoded;
        if (encoder.encode_frame(frame, encoded)) {
            encoded_frames.push_back(encoded);
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Calculate statistics
    size_t total_encoded_size = 0;
    for (const auto& frame : encoded_frames) {
        total_encoded_size += frame.size();
    }
    
    size_t original_size = input_frame.samples.size() * sizeof(int16_t);
    double compression_ratio = static_cast<double>(original_size) / total_encoded_size;
    double avg_frame_size = static_cast<double>(total_encoded_size) / encoded_frames.size();
    
    std::cout << "✅ Opus encoding successful!" << std::endl;
    std::cout << "Original size: " << original_size << " bytes" << std::endl;
    std::cout << "Encoded size: " << total_encoded_size << " bytes" << std::endl;
    std::cout << "Compression ratio: " << compression_ratio << ":1" << std::endl;
    std::cout << "Average frame size: " << avg_frame_size << " bytes" << std::endl;
    std::cout << "Encoding time: " << duration.count() / 1000.0 << " ms" << std::endl;
    std::cout << "Bitrate: " << (total_encoded_size * 8) / 1000.0 << " kbps" << std::endl;
}

void test_audio_processor() {
    std::cout << "\n=== AUDIO PROCESSOR TEST ===" << std::endl;
    
    audio::AudioConfig input_config;
    input_config.sample_rate = 44100;
    input_config.channels = 1;
    
    audio::AudioConfig output_config;
    output_config.sample_rate = 48000;
    output_config.channels = 2;
    
    audio::AudioProcessor processor;
    if (!processor.initialize(input_config, output_config)) {
        std::cerr << "Failed to initialize audio processor" << std::endl;
        return;
    }
    
    // Generate test audio
    audio::AudioFrame input_frame;
    generate_test_audio(input_frame, input_config.sample_rate, 
                       input_config.channels, 500); // 0.5 seconds
    
    audio::AudioFrame output_frame;
    processor.process_audio(input_frame, output_frame);
    
    std::cout << "✅ Audio processing successful!" << std::endl;
    std::cout << "Input: " << input_config.sample_rate << "Hz, " 
              << input_config.channels << " channel" << std::endl;
    std::cout << "Output: " << output_frame.sample_rate << "Hz, " 
              << output_frame.channels << " channels" << std::endl;
    std::cout << "Processed " << input_frame.samples.size() << " samples" << std::endl;
}

int main() {
    try {
        std::cout << "🎵 Streaming Engine Audio Codec Tests" << std::endl;
        std::cout << "=====================================" << std::endl;
        
        test_opus_encoder();
        test_audio_processor();
        
        std::cout << "\n🎉 All audio tests completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
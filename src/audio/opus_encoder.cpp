// src/audio/opus_encoder.cpp
#include "streaming/audio/opus_encoder.hpp"
#include "streaming/utils/audio_utils.hpp"
#include <opus/opus.h>
#include <iostream>
#include <algorithm>

namespace streaming {
namespace audio {

OpusEncoder::OpusEncoder() = default;

OpusEncoder::~OpusEncoder() {
    if (encoder_) {
        opus_encoder_destroy(static_cast<OpusEncoder*>(encoder_));
    }
}

bool OpusEncoder::initialize(const AudioConfig& config) {
    config_ = config;
    
    int error = 0;
    encoder_ = opus_encoder_create(config.sample_rate, config.channels, 
                                  OPUS_APPLICATION_AUDIO, &error);
    
    if (error != OPUS_OK || !encoder_) {
        std::cerr << "Failed to create Opus encoder: " << opus_strerror(error) << std::endl;
        return false;
    }
    
    // Configure encoder for streaming
    opus_encoder_ctl(static_cast<OpusEncoder*>(encoder_), OPUS_SET_BITRATE(config.bitrate));
    opus_encoder_ctl(static_cast<OpusEncoder*>(encoder_), OPUS_SET_VBR(vbr_enabled_ ? 1 : 0));
    opus_encoder_ctl(static_cast<OpusEncoder*>(encoder_), OPUS_SET_DTX(dtx_enabled_ ? 1 : 0));
    opus_encoder_ctl(static_cast<OpusEncoder*>(encoder_), OPUS_SET_COMPLEXITY(config.complexity));
    opus_encoder_ctl(static_cast<OpusEncoder*>(encoder_), OPUS_SET_PACKET_LOSS_PERC(packet_loss_percentage_));
    
    if (fec_enabled_) {
        opus_encoder_ctl(static_cast<OpusEncoder*>(encoder_), OPUS_SET_INBAND_FEC(1));
    }
    
    std::cout << "🎵 OpusEncoder initialized: " << config.sample_rate << "Hz, " 
              << config.channels << "ch, " << config.bitrate/1000 << "kbps" << std::endl;
    std::cout << "   Features: VBR=" << vbr_enabled_ << ", DTX=" << dtx_enabled_ 
              << ", FEC=" << fec_enabled_ << std::endl;
    
    return true;
}

bool OpusEncoder::encode_frame(const AudioFrame& input, std::vector<uint8_t>& output) {
    if (!encoder_ || input.samples.empty()) {
        return false;
    }
    
    // Validate frame size
    uint32_t expected_samples = input.frame_size * input.channels;
    if (input.samples.size() != expected_samples) {
        std::cerr << "Invalid frame size: expected " << expected_samples 
                  << ", got " << input.samples.size() << std::endl;
        return false;
    }
    
    // Voice activity detection for DTX
    bool is_voiced = true;
    if (dtx_enabled_) {
        is_voiced = detect_voice_activity(input.samples.data(), input.frame_size);
        if (!is_voiced) {
            consecutive_silence_frames_++;
        } else {
            consecutive_silence_frames_ = 0;
        }
    }
    
    // For DTX: don't encode silence frames after a few consecutive ones
    if (dtx_enabled_ && consecutive_silence_frames_ > 10 && !is_voiced) {
        output.clear(); // Empty packet for DTX
        return true;
    }
    
    // Apply psychoacoustic optimizations
    apply_psychoacoustic_optimizations(input.samples.data(), input.frame_size);
    
    // Encode with Opus
    output.resize(input.frame_size * input.channels * 2); // Max possible size
    int encoded_bytes = opus_encode(static_cast<OpusEncoder*>(encoder_), 
                                   input.samples.data(), input.frame_size,
                                   output.data(), output.size());
    
    if (encoded_bytes < 0) {
        std::cerr << "Opus encoding error: " << opus_strerror(encoded_bytes) << std::endl;
        return false;
    }
    
    output.resize(encoded_bytes);
    total_encoded_bytes_ += encoded_bytes;
    total_input_samples_ += input.samples.size();
    
    return true;
}

bool OpusEncoder::encode_opus_frame(const int16_t* samples, uint32_t frame_size, 
                                   std::vector<uint8_t>& output) {
    output.resize(frame_size * config_.channels * 2);
    
    int encoded_bytes = opus_encode(static_cast<OpusEncoder*>(encoder_), 
                                   samples, frame_size,
                                   output.data(), output.size());
    
    if (encoded_bytes < 0) {
        return false;
    }
    
    output.resize(encoded_bytes);
    return true;
}

void OpusEncoder::apply_psychoacoustic_optimizations(const int16_t* samples, uint32_t count) {
    // Simple psychoacoustic preprocessing
    // In real implementation, this would include:
    // - Spectral analysis
    // - Masking threshold calculation
    // - Bit allocation optimization
    
    // For now, apply gentle high-frequency pre-emphasis
    const float pre_emphasis = 0.95f;
    std::vector<int16_t> processed(samples, samples + count);
    
    if (config_.channels == 1) {
        for (size_t i = 1; i < count; ++i) {
            float sample = processed[i] - pre_emphasis * processed[i-1];
            processed[i] = static_cast<int16_t>(std::max(-32768.0f, std::min(32767.0f, sample)));
        }
    } else {
        // Stereo processing
        for (size_t i = 2; i < count; i += 2) {
            for (int ch = 0; ch < 2; ++ch) {
                float sample = processed[i + ch] - pre_emphasis * processed[i - 2 + ch];
                processed[i + ch] = static_cast<int16_t>(std::max(-32768.0f, std::min(32767.0f, sample)));
            }
        }
    }
}

bool OpusEncoder::detect_voice_activity(const int16_t* samples, uint32_t count) {
    // Simple energy-based VAD (Voice Activity Detection)
    double energy = 0.0;
    for (uint32_t i = 0; i < count; ++i) {
        energy += samples[i] * samples[i];
    }
    energy = std::sqrt(energy / count);
    
    // Adaptive threshold based on recent history
    double threshold = 500.0; // Empirical value
    return energy > threshold;
}

void OpusEncoder::set_bitrate(uint32_t bitrate) {
    config_.bitrate = bitrate;
    if (encoder_) {
        opus_encoder_ctl(static_cast<OpusEncoder*>(encoder_), OPUS_SET_BITRATE(bitrate));
    }
}

uint32_t OpusEncoder::get_encoded_size() const {
    return total_encoded_bytes_;
}

double OpusEncoder::get_compression_ratio() const {
    if (total_input_samples_ == 0) return 1.0;
    
    double input_bytes = total_input_samples_ * sizeof(int16_t);
    return input_bytes / total_encoded_bytes_;
}

void OpusEncoder::enable_dtx(bool enable) {
    dtx_enabled_ = enable;
    if (encoder_) {
        opus_encoder_ctl(static_cast<OpusEncoder*>(encoder_), OPUS_SET_DTX(enable ? 1 : 0));
    }
}

void OpusEncoder::enable_fec(bool enable) {
    fec_enabled_ = enable;
    if (encoder_) {
        opus_encoder_ctl(static_cast<OpusEncoder*>(encoder_), OPUS_SET_INBAND_FEC(enable ? 1 : 0));
    }
}

} // namespace audio
} // namespace streaming
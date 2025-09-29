#include "media/codecs.hpp"
#include <spdlog/spdlog.h>
#include <cstring>

using namespace media;

// --- PCM Audio Encoder ---
std::vector<uint8_t> PCMEncoder::encode(const std::vector<uint8_t>& data) {
    // PCM: just return raw data (no compression)
    spdlog::debug("PCM encoding {} bytes", data.size());
    return data;
}

// --- Simple Bitmap Encoder ---
std::vector<uint8_t> BitmapEncoder::encode(const std::vector<uint8_t>& data) {
    // Simple bitmap encoding - just add a header
    std::vector<uint8_t> out;
    
    // Add a simple header
    out.push_back('B');
    out.push_back('M');
    
    // Copy the data
    out.insert(out.end(), data.begin(), data.end());
    
    spdlog::debug("Bitmap encoding {} -> {} bytes", data.size(), out.size());
    return out;
}

// --- Delta Encoding Encoder ---
std::vector<uint8_t> DeltaEncoder::encode(const std::vector<uint8_t>& data) {
    if (previous_frame_.empty()) {
        previous_frame_ = data;
        spdlog::debug("Delta encoding (first frame) {} bytes", data.size());
        return data; // First frame, no delta
    }
    
    return encode_with_reference(data, previous_frame_);
}

std::vector<uint8_t> DeltaEncoder::encode_with_reference(const std::vector<uint8_t>& data, const std::vector<uint8_t>& prev_frame) {
    if (data.size() != prev_frame.size()) {
        spdlog::warn("Frame size changed, cannot compute delta");
        previous_frame_ = data;
        return data;
    }
    
    std::vector<uint8_t> delta;
    for (size_t i = 0; i < data.size(); i++) {
        int16_t diff = static_cast<int16_t>(data[i]) - static_cast<int16_t>(prev_frame[i]);
        delta.push_back(static_cast<uint8_t>(diff & 0xFF));
    }
    
    previous_frame_ = data;
    spdlog::debug("Delta encoding {} bytes", delta.size());
    return delta;
}

// --- Dummy JPEG-like Encoder ---
std::vector<uint8_t> JPEGEncoder::encode(const std::vector<uint8_t>& data) {
    // Simple JPEG-like encoding - add headers
    std::vector<uint8_t> out = {0xFF, 0xD8}; // SOI
    out.insert(out.end(), data.begin(), data.end());
    out.push_back(0xFF);
    out.push_back(0xD9); // EOI
    
    spdlog::debug("JPEG encoding {} -> {} bytes", data.size(), out.size());
    return out;
}

// --- Dummy H264-like Encoder ---
std::vector<uint8_t> H264Encoder::encode(const std::vector<uint8_t>& data) {
    // Simple H264-like encoding - add NAL header
    std::vector<uint8_t> out = {0x00, 0x00, 0x00, 0x01}; // NAL start
    out.insert(out.end(), data.begin(), data.end());
    
    spdlog::debug("H264 encoding {} -> {} bytes", data.size(), out.size());
    return out;
}

// --- Dummy VP8-like Encoder ---
std::vector<uint8_t> VP8Encoder::encode(const std::vector<uint8_t>& data) {
    // Simple VP8-like encoding - add header
    std::vector<uint8_t> out = {'V', 'P', '8', ' '};
    out.insert(out.end(), data.begin(), data.end());
    
    spdlog::debug("VP8 encoding {} -> {} bytes", data.size(), out.size());
    return out;
}

// --- Dummy AV1-like Encoder ---
std::vector<uint8_t> AV1Encoder::encode(const std::vector<uint8_t>& data) {
    // Simple AV1-like encoding - add header
    std::vector<uint8_t> out = {'A', 'V', '1', ' '};
    out.insert(out.end(), data.begin(), data.end());
    
    spdlog::debug("AV1 encoding {} -> {} bytes", data.size(), out.size());
    return out;
}

// --- PCM Audio Decoder ---
std::vector<uint8_t> PCMDecoder::decode(const std::vector<uint8_t>& data) {
    // PCM: just return raw data (no decompression needed)
    spdlog::debug("PCM decoding {} bytes", data.size());
    return data;
}

// --- Simple Bitmap Decoder ---
std::vector<uint8_t> BitmapDecoder::decode(const std::vector<uint8_t>& data) {
    // Simple bitmap decoding - remove header
    if (data.size() < 2 || data[0] != 'B' || data[1] != 'M') {
        spdlog::warn("Invalid bitmap data");
        return data;
    }
    
    std::vector<uint8_t> out(data.begin() + 2, data.end());
    spdlog::debug("Bitmap decoding {} -> {} bytes", data.size(), out.size());
    return out;
}

// --- Delta Encoding Decoder ---
std::vector<uint8_t> DeltaDecoder::decode(const std::vector<uint8_t>& data) {
    if (previous_frame_.empty()) {
        // First frame, store it
        previous_frame_ = data;
        spdlog::debug("Delta decoding (first frame) {} bytes", data.size());
        return data;
    }
    
    return decode_with_reference(data, previous_frame_);
}

std::vector<uint8_t> DeltaDecoder::decode_with_reference(const std::vector<uint8_t>& data, const std::vector<uint8_t>& prev_frame) {
    if (data.size() != prev_frame.size()) {
        spdlog::warn("Frame size mismatch in delta decoding");
        return data;
    }
    
    std::vector<uint8_t> reconstructed;
    for (size_t i = 0; i < data.size(); i++) {
        int16_t value = static_cast<int16_t>(prev_frame[i]) + static_cast<int16_t>(data[i]);
        reconstructed.push_back(static_cast<uint8_t>(value & 0xFF));
    }
    
    previous_frame_ = reconstructed;
    spdlog::debug("Delta decoding {} bytes", reconstructed.size());
    return reconstructed;
}

// --- Dummy JPEG-like Decoder ---
std::vector<uint8_t> JPEGDecoder::decode(const std::vector<uint8_t>& data) {
    // Simple JPEG-like decoding - remove headers
    if (data.size() < 4 || data[0] != 0xFF || data[1] != 0xD8) {
        spdlog::warn("Invalid JPEG data");
        return data;
    }
    
    // Remove SOI and EOI markers
    std::vector<uint8_t> out(data.begin() + 2, data.end() - 2);
    spdlog::debug("JPEG decoding {} -> {} bytes", data.size(), out.size());
    return out;
}

// --- Dummy H264-like Decoder ---
std::vector<uint8_t> H264Decoder::decode(const std::vector<uint8_t>& data) {
    // Simple H264-like decoding - remove NAL header
    if (data.size() < 4 || data[0] != 0x00 || data[1] != 0x00 || data[2] != 0x00 || data[3] != 0x01) {
        spdlog::warn("Invalid H264 data");
        return data;
    }
    
    std::vector<uint8_t> out(data.begin() + 4, data.end());
    spdlog::debug("H264 decoding {} -> {} bytes", data.size(), out.size());
    return out;
}

// --- Dummy VP8-like Decoder ---
std::vector<uint8_t> VP8Decoder::decode(const std::vector<uint8_t>& data) {
    // Simple VP8-like decoding - remove header
    if (data.size() < 4 || data[0] != 'V' || data[1] != 'P' || data[2] != '8') {
        spdlog::warn("Invalid VP8 data");
        return data;
    }
    
    std::vector<uint8_t> out(data.begin() + 4, data.end());
    spdlog::debug("VP8 decoding {} -> {} bytes", data.size(), out.size());
    return out;
}

// --- Dummy AV1-like Decoder ---
std::vector<uint8_t> AV1Decoder::decode(const std::vector<uint8_t>& data) {
    // Simple AV1-like decoding - remove header
    if (data.size() < 4 || data[0] != 'A' || data[1] != 'V' || data[2] != '1') {
        spdlog::warn("Invalid AV1 data");
        return data;
    }
    
    std::vector<uint8_t> out(data.begin() + 4, data.end());
    spdlog::debug("AV1 decoding {} -> {} bytes", data.size(), out.size());
    return out;
}
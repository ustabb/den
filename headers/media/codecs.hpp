#pragma once

#include <vector>
#include <cstdint>
#include "encoder.hpp"
#include "decoder.hpp"
#include "frame.hpp"

namespace media {

// --- PCM Audio Codec ---
class PCMEncoder : public Encoder {
public:
    std::vector<uint8_t> encode(const std::vector<uint8_t>& data) override;
};

// --- Simple Bitmap Codec (RGB24) ---
class BitmapEncoder : public Encoder {
public:
    std::vector<uint8_t> encode(const std::vector<uint8_t>& data) override;
};

// --- Delta Encoding Codec ---
class DeltaEncoder : public Encoder {
private:
    std::vector<uint8_t> previous_frame_;
    
public:
    std::vector<uint8_t> encode(const std::vector<uint8_t>& data) override;
    std::vector<uint8_t> encode_with_reference(const std::vector<uint8_t>& data, const std::vector<uint8_t>& prev_frame);
};

// --- Dummy JPEG-like Encoder (stub) ---
class JPEGEncoder : public Encoder {
public:
    std::vector<uint8_t> encode(const std::vector<uint8_t>& data) override;
};

// --- Dummy H264-like Encoder (stub) ---
class H264Encoder : public Encoder {
public:
    std::vector<uint8_t> encode(const std::vector<uint8_t>& data) override;
};

// --- Dummy VP8-like Encoder (stub) ---
class VP8Encoder : public Encoder {
public:
    std::vector<uint8_t> encode(const std::vector<uint8_t>& data) override;
};

// --- Dummy AV1-like Encoder (stub) ---
class AV1Encoder : public Encoder {
public:
    std::vector<uint8_t> encode(const std::vector<uint8_t>& data) override;
};

// --- PCM Audio Decoder ---
class PCMDecoder : public Decoder {
public:
    std::vector<uint8_t> decode(const std::vector<uint8_t>& data) override;
};

// --- Simple Bitmap Decoder ---
class BitmapDecoder : public Decoder {
public:
    std::vector<uint8_t> decode(const std::vector<uint8_t>& data) override;
};

// --- Delta Encoding Decoder ---
class DeltaDecoder : public Decoder {
private:
    std::vector<uint8_t> previous_frame_;
    
public:
    std::vector<uint8_t> decode(const std::vector<uint8_t>& data) override;
    std::vector<uint8_t> decode_with_reference(const std::vector<uint8_t>& data, const std::vector<uint8_t>& prev_frame);
};

// --- Dummy JPEG-like Decoder (stub) ---
class JPEGDecoder : public Decoder {
public:
    std::vector<uint8_t> decode(const std::vector<uint8_t>& data) override;
};

// --- Dummy H264-like Decoder (stub) ---
class H264Decoder : public Decoder {
public:
    std::vector<uint8_t> decode(const std::vector<uint8_t>& data) override;
};

// --- Dummy VP8-like Decoder (stub) ---
class VP8Decoder : public Decoder {
public:
    std::vector<uint8_t> decode(const std::vector<uint8_t>& data) override;
};

// --- Dummy AV1-like Decoder (stub) ---
class AV1Decoder : public Decoder {
public:
    std::vector<uint8_t> decode(const std::vector<uint8_t>& data) override;
};

} // namespace media
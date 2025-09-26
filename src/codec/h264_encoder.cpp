// src/codec/h264_encoder.cpp - TAMAMEN YENİ
#include "streaming/codec/h264_encoder.hpp"
#include "streaming/processing/dct_transform.hpp"
#include "streaming/processing/quantization.hpp"
#include "streaming/processing/cavlc_encoder.hpp"
#include "streaming/processing/motion_estimation.hpp"
#include <iostream>
#include <cmath>

namespace streaming {
namespace codec {

H264Encoder::H264Encoder() 
    : dct_(std::make_unique<processing::DCT>())
    , quantizer_(std::make_unique<processing::Quantizer>())
    , cavlc_encoder_(std::make_unique<processing::CAVLCEncoder>()) {}

bool H264Encoder::initialize(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate) {
    width_ = width;
    height_ = height;
    fps_ = fps;
    bitrate_ = bitrate;
    frame_count_ = 0;
    
    // Calculate target bits per frame
    target_bits_per_frame_ = bitrate / fps;
    
    // Initialize reference frames buffer
    reference_frames_.resize(width * height * 1.5); // YUV420
    
    std::cout << "🚀 H264Encoder initialized: " << width << "x" << height 
              << " @" << fps << "fps, " << bitrate << "bps" << std::endl;
    
    return true;
}

bool H264Encoder::encode_frame(const VideoFrame& input, std::vector<uint8_t>& output) {
    utils::BitstreamWriter writer;
    
    try {
        // Encode NAL unit
        if (!encode_nal_unit(input, writer)) {
            return false;
        }
        
        output = writer.get_data();
        frame_count_++;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Encoding error: " << e.what() << std::endl;
        return false;
    }
}

bool H264Encoder::encode_nal_unit(const VideoFrame& frame, utils::BitstreamWriter& writer) {
    // NAL unit start code
    writer.write_bits(0x00000001, 32);
    
    // NAL header
    bool forbidden_zero_bit = false;
    uint8_t nal_ref_idc = (frame_count_ % gop_size_ == 0) ? 3 : 2;
    uint8_t nal_unit_type = (frame_count_ % gop_size_ == 0) ? 5 : 1; // I-frame or P-frame
    
    writer.write_bit(forbidden_zero_bit);
    writer.write_bits(nal_ref_idc, 2);
    writer.write_bits(nal_unit_type, 5);
    
    // Slice header
    encode_slice_header(writer, nal_unit_type);
    
    // Encode slice data
    encode_slice_data(writer, frame, nal_unit_type);
    
    return true;
}

void H264Encoder::encode_slice_header(utils::BitstreamWriter& writer, uint8_t slice_type) {
    writer.write_ue(0); // first_mb_in_slice
    writer.write_ue(slice_type == 5 ? 2 : 0); // slice_type (0=P, 2=I)
    writer.write_ue(0); // pic_parameter_set_id
    writer.write_se(0); // frame_num
    
    if (slice_type == 5) { // IDR frame
        writer.write_ue(0); // idr_pic_id
    }
    
    writer.write_ue(current_qp_); // slice_qp_delta
}

void H264Encoder::encode_slice_data(utils::BitstreamWriter& writer, const VideoFrame& frame, uint8_t slice_type) {
    uint32_t mb_width = (width_ + 15) / 16;
    uint32_t mb_height = (height_ + 15) / 16;
    
    processing::MotionEstimator motion_estimator;
    
    for (uint32_t mb_y = 0; mb_y < mb_height; ++mb_y) {
        for (uint32_t mb_x = 0; mb_x < mb_width; ++mb_x) {
            // Encode macroblock
            encode_macroblock(writer, frame, mb_x, mb_y, slice_type, motion_estimator);
        }
    }
}

void H264Encoder::encode_macroblock(utils::BitstreamWriter& writer, const VideoFrame& frame, 
                                   uint32_t mb_x, uint32_t mb_y, uint8_t slice_type,
                                   processing::MotionEstimator& motion_estimator) {
    Macroblock mb;
    extract_macroblock(frame, mb, mb_x, mb_y);
    
    if (slice_type == 5) { // I-frame - Intra prediction
        writer.write_ue(1); // I_PCM or I_16x16
        encode_intra_macroblock(writer, mb);
    } else { // P-frame - Inter prediction
        // Motion estimation
        auto mv = motion_estimator.estimate_diamond_search(
            frame.data.data(), reference_frames_.data(), width_, height_, 
            mb_x * 16, mb_y * 16
        );
        
        if (mv.valid && mv.cost < 1000) { // Use motion compensation
            writer.write_ue(0); // P_L0_16x16
            encode_motion_vector(writer, mv);
            encode_residual(writer, mb); // Encode residual
        } else { // Fallback to intra
            writer.write_ue(1); // Intra
            encode_intra_macroblock(writer, mb);
        }
    }
    
    // Store as reference for future frames
    store_macroblock_reference(mb, mb_x, mb_y);
}

void H264Encoder::encode_intra_macroblock(utils::BitstreamWriter& writer, const Macroblock& mb) {
    // Perform DCT and quantization
    Macroblock transformed_mb = mb;
    perform_dct_quantization(transformed_mb);
    
    // Encode each 8x8 block
    for (const auto& block : transformed_mb.y_blocks) {
        cavlc_encoder_->encode_residual(writer, block);
    }
}

void H264Encoder::encode_motion_vector(utils::BitstreamWriter& writer, const processing::MotionVector& mv) {
    writer.write_se(mv.x); // Motion vector difference X
    writer.write_se(mv.y); // Motion vector difference Y
}

void H264Encoder::encode_residual(utils::BitstreamWriter& writer, const Macroblock& mb) {
    // Encode residual after motion compensation
    for (const auto& block : mb.y_blocks) {
        cavlc_encoder_->encode_residual(writer, block);
    }
}

void H264Encoder::extract_macroblock(const VideoFrame& frame, Macroblock& mb, uint32_t mb_x, uint32_t mb_y) {
    // Extract Y component (luma)
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            uint32_t px = mb_x * 16 + x;
            uint32_t py = mb_y * 16 + y;
            
            if (px < width_ && py < height_) {
                uint32_t idx = py * width_ + px;
                mb.y_blocks[y/8][x/8][y%8][x%8] = frame.data[idx] - 128; // Center around 0
            }
        }
    }
    
    // Extract UV components (chroma) - simplified
    // Gerçek implementasyonda UV downsampling yapılacak
}

void H264Encoder::perform_dct_quantization(Macroblock& mb) {
    for (auto& block : mb.y_blocks) {
        std::array<std::array<double, 8>, 8> dct_coeffs;
        
        // Forward DCT
        dct_->forward_dct(block, dct_coeffs);
        
        // Quantization
        quantizer_->quantize_block(dct_coeffs, current_qp_);
        
        // Convert back to integer (for encoding)
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) {
                block[i][j] = static_cast<int16_t>(std::round(dct_coeffs[i][j]));
            }
        }
    }
}

void H264Encoder::store_macroblock_reference(const Macroblock& mb, uint32_t mb_x, uint32_t mb_y) {
    // Store macroblock as reference for future frames
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            uint32_t px = mb_x * 16 + x;
            uint32_t py = mb_y * 16 + y;
            
            if (px < width_ && py < height_) {
                uint32_t idx = py * width_ + px;
                reference_frames_[idx] = static_cast<uint8_t>(mb.y_blocks[y/8][x/8][y%8][x%8] + 128);
            }
        }
    }
}

void H264Encoder::set_bitrate(uint32_t bitrate) {
    bitrate_ = bitrate;
    target_bits_per_frame_ = bitrate / fps_;
    
    
    
    
    
    
    // Adjust QP based on bitrate
    current_qp_ = 26 + (bitrate < 2000000 ? 5 : 0) - (bitrate > 5000000 ? 5 : 0);
    current_qp_ = std::max(10, std::min(40, current_qp_));
}

void H264Encoder::set_gop_size(uint32_t gop_size) {
    gop_size_ = std::max(1u, gop_size);
}

uint32_t H264Encoder::get_encoded_size() const {
    return frame_count_ * target_bits_per_frame_ / 8; // Estimated size
}

} // namespace codec
} // namespace streaming
// src/codec/h265_encoder.cpp
#include "streaming/codec/h265_encoder.hpp"
#include "streaming/processing/cabac_encoder.hpp"
#include "streaming/processing/motion_estimation.hpp"
#include <iostream>
#include <cmath>

namespace streaming {
namespace codec {

H265Encoder::H265Encoder() 
    : dct_(std::make_unique<processing::DCT>())
    , quantizer_(std::make_unique<processing::Quantizer>()) {}

bool H265Encoder::initialize(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate) {
    width_ = width;
    height_ = height;
    fps_ = fps;
    bitrate_ = bitrate;
    frame_count_ = 0;
    
    // HEVC-specific initialization
    ctu_size_ = 64; // HEVC allows 16, 32, 64
    max_cu_depth_ = std::log2(ctu_size_ / 8); // Up to 8x8 CUs
    
    // Calculate target bits per frame
    target_bits_per_frame_ = bitrate / fps;
    current_qp_ = 32; // HEVC QP range: 0-51
    
    // Initialize reference frames buffer
    reference_frames_.resize(width * height * 3 / 2); // YUV420
    
    std::cout << "🚀 H265Encoder initialized: " << width << "x" << height 
              << " @" << fps << "fps, " << bitrate << "bps" << std::endl;
    std::cout << "   CTU Size: " << ctu_size_ << ", Max CU Depth: " << max_cu_depth_ << std::endl;
    
    return true;
}

bool H265Encoder::encode_frame(const VideoFrame& input, std::vector<uint8_t>& output) {
    utils::BitstreamWriter writer;
    
    try {
        // Encode HEVC NAL unit
        if (!encode_nal_unit(input, writer)) {
            return false;
        }
        
        output = writer.get_data();
        frame_count_++;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "H.265 Encoding error: " << e.what() << std::endl;
        return false;
    }
}

bool H265Encoder::encode_nal_unit(const VideoFrame& frame, utils::BitstreamWriter& writer) {
    bool is_idr = (frame_count_ % gop_size_ == 0);
    
    // HEVC NAL unit header (2 bytes)
    writer.write_bit(0); // forbidden_zero_bit
    writer.write_bits(0, 6); // nal_unit_type (simplified)
    writer.write_bits(is_idr ? 19 : 1, 6); // HEVC NAL unit types
    writer.write_bits(0, 6); // nuh_temporal_id_plus1
    
    // Slice header
    encode_slice_header(writer, is_idr);
    
    // Encode CTUs (Coding Tree Units)
    int ctus_width = (width_ + ctu_size_ - 1) / ctu_size_;
    int ctus_height = (height_ + ctu_size_ - 1) / ctu_size_;
    
    for (int y = 0; y < ctus_height; ++y) {
        for (int x = 0; x < ctus_width; ++x) {
            CTU ctu;
            // Extract CTU from frame (simplified)
            encode_ctu(writer, ctu, x * ctu_size_, y * ctu_size_);
        }
    }
    
    // HEVC in-loop filters
    encode_sao_parameters(writer);
    encode_deblocking_params(writer);
    
    return true;
}

void H265Encoder::encode_slice_header(utils::BitstreamWriter& writer, bool is_idr) {
    writer.write_ue(0); // first_slice_segment_in_pic_flag
    writer.write_ue(is_idr ? 2 : 0); // slice_type (0=P, 2=I)
    writer.write_ue(0); // pic_parameter_set_id
    
    if (is_idr) {
        writer.write_ue(frame_count_); // idr_pic_id
    }
    
    writer.write_ue(current_qp_); // slice_qp_delta
    
    // HEVC specific parameters
    writer.write_bit(0); // dependent_slice_segment_flag
    writer.write_ue(0); // slice_segment_address
}

void H265Encoder::encode_ctu(utils::BitstreamWriter& writer, const CTU& ctu, int x, int y) {
    std::vector<CodingUnit> coding_units;
    
    // Rate-Distortion Optimized CTU splitting decision
    rdo_ctu_split_decision(const_cast<CTU&>(ctu), x, y, coding_units);
    
    // Encode quad-tree structure
    for (const auto& cu : coding_units) {
        encode_coding_unit(writer, cu);
    }
}

void H265Encoder::rdo_ctu_split_decision(CTU& ctu, int x, int y, std::vector<CodingUnit>& cus) {
    // HEVC uses quad-tree structure for CTU splitting
    // This is a simplified RDO-based splitting decision
    
    for (int depth = 0; depth <= max_cu_depth_; ++depth) {
        int cu_size = ctu_size_ >> depth;
        
        for (int cu_y = y; cu_y < y + ctu_size_; cu_y += cu_size) {
            for (int cu_x = x; cu_x < x + ctu_size_; cu_x += cu_size) {
                CodingUnit cu;
                cu.x = cu_x;
                cu.y = cu_y;
                cu.size = cu_size;
                
                // Simplified RDO cost calculation
                double cost_no_split = calculate_cu_cost(ctu, cu, false);
                double cost_split = calculate_cu_cost(ctu, cu, true);
                
                cu.split = (cost_split < cost_no_split) && (depth < max_cu_depth_);
                cus.push_back(cu);
                
                if (!cu.split) {
                    // Encode this CU without further splitting
                    break;
                }
            }
        }
    }
}

double H265Encoder::calculate_cu_cost(const CTU& ctu, const CodingUnit& cu, bool split) {
    // Simplified Rate-Distortion cost calculation
    // In real implementation, this would involve actual encoding and distortion measurement
    
    double distortion = 0.0;
    double rate = 0.0;
    double lambda = 0.85 * std::pow(2.0, (current_qp_ - 12) / 3.0);
    
    if (split) {
        // Cost of signaling split flags + smaller CUs
        rate = 1.0 + (4 * 0.5); // Split flag + 4 smaller CUs
        distortion = 50.0; // Estimated distortion when splitting
    } else {
        // Cost of encoding this CU directly
        rate = 10.0; // More bits for larger CU
        distortion = 30.0; // Less distortion for larger CU
    }
    
    return distortion + lambda * rate;
}

void H265Encoder::encode_coding_unit(utils::BitstreamWriter& writer, const CodingUnit& cu) {
    // Encode split flag
    writer.write_bit(cu.split ? 1 : 0);
    
    if (!cu.split) {
        // Encode prediction mode
        bool is_intra = (frame_count_ % gop_size_ == 0); // Simplified
        writer.write_bit(is_intra ? 1 : 0);
        
        if (is_intra) {
            encode_intra_prediction(writer, cu);
        } else {
            encode_inter_prediction(writer, cu);
        }
        
        // Encode residual
        encode_residual_quadtree(writer, cu.tu);
    }
}

void H265Encoder::encode_intra_prediction(utils::BitstreamWriter& writer, const CodingUnit& cu) {
    // HEVC has 35 intra prediction modes
    writer.write_ue(0); // Simplified: always use DC mode
    
    // Encode intra prediction details
    if (cu.size > 8) {
        writer.write_bit(0); // not intra_luma_mpm_flag
        writer.write_ue(0); // intra_luma_mpm_idx
    }
}

void H265Encoder::encode_inter_prediction(utils::BitstreamWriter& writer, const CodingUnit& cu) {
    processing::MotionEstimator motion_est;
    
    // Motion estimation for this CU
    auto mv = motion_est.estimate_diamond_search(
        nullptr, reference_frames_.data(), width_, height_, // Simplified
        cu.x, cu.y
    );
    
    if (mv.valid) {
        writer.write_bit(1); // use_inter_prediction
        writer.write_se(mv.x); // mv_diff_x
        writer.write_se(mv.y); // mv_diff_y
        writer.write_ue(0); // ref_idx_l0
    } else {
        writer.write_bit(0); // fallback to intra
        encode_intra_prediction(writer, cu);
    }
}

void H265Encoder::encode_residual_quadtree(utils::BitstreamWriter& writer, const TransformUnit& tu) {
    // HEVC uses residual quad-tree (RQT) for transform unit splitting
    bool split_transform_flag = (tu.transform_size > 8); // Simplified
    writer.write_bit(split_transform_flag ? 1 : 0);
    
    if (!split_transform_flag) {
        // Encode transform coefficients for this TU
        for (int i = 0; i < tu.transform_size; ++i) {
            for (int j = 0; j < tu.transform_size; ++j) {
                if (tu.coeffs[i][j] != 0) {
                    writer.write_se(tu.coeffs[i][j]);
                }
            }
        }
    }
}

void H265Encoder::encode_sao_parameters(utils::BitstreamWriter& writer) {
    // Sample Adaptive Offset - HEVC's advanced in-loop filter
    writer.write_bit(1); // slice_sao_luma_flag
    writer.write_bit(1); // slice_sao_chroma_flag
    
    // Simplified SAO parameters
    writer.write_ue(0); // sao_type_idx_luma (0=OFF, 1=BO, 2=EO)
    writer.write_ue(0); // sao_type_idx_chroma
}

void H265Encoder::encode_deblocking_params(utils::BitstreamWriter& writer) {
    // Deblocking filter parameters
    writer.write_bit(1); // deblocking_filter_override_flag
    writer.write_bit(0); // pic_disable_deblocking_filter_flag
    
    if (true) { // deblocking_filter_enabled
        writer.write_se(0); // beta_offset_div2
        writer.write_se(0); // tc_offset_div2
    }
}

void H265Encoder::set_bitrate(uint32_t bitrate) {
    bitrate_ = bitrate;
    target_bits_per_frame_ = bitrate / fps_;
    
    // HEVC QP adjustment based on bitrate
    current_qp_ = 32 + (bitrate < 1500000 ? 6 : 0) - (bitrate > 3000000 ? 6 : 0);
    current_qp_ = std::max(22, std::min(42, current_qp_));
}

void H265Encoder::set_gop_size(uint32_t gop_size) {
    gop_size_ = std::max(1u, gop_size);
}

uint32_t H265Encoder::get_encoded_size() const {
    return frame_count_ * target_bits_per_frame_ / 8;
}

} // namespace codec
} // namespace streaming
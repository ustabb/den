// src/codec/vvc_encoder.cpp
#include "streaming/codec/vvc_encoder.hpp"
#include "streaming/processing/vvc_entropy.hpp"
#include "streaming/processing/motion_estimation.hpp"
#include <iostream>
#include <cmath>
#include <thread>
#include <future>

namespace streaming {
namespace codec {

VVCEncoder::VVCEncoder() 
    : dct_(std::make_unique<processing::DCT>())
    , quantizer_(std::make_unique<processing::Quantizer>()) {
    
    // Default VVC advanced features
    features_.bdpcm_enabled = true;
    features_.mip_enabled = true;
    features_.affine_enabled = true;
    features_.cclm_enabled = true;
    features_.ibc_enabled = true;
    features_.palette_enabled = true;
}

bool VVCEncoder::initialize(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate) {
    width_ = width;
    height_ = height;
    fps_ = fps;
    bitrate_ = bitrate;
    frame_count_ = 0;
    
    // VVC-specific initialization
    ctu_size_ = 128; // VVC supports up to 256x256 CTUs!
    max_mtt_depth_ = 4;
    current_qp_ = 35;
    
    target_bits_per_frame_ = bitrate / fps;
    
    // Initialize reference frames and IBC buffer
    reference_frames_.resize(width * height * 3 / 2);
    ibc_buffer_.resize(width * height);
    
    std::cout << "🚀 VVCEncoder initialized: " << width << "x" << height 
              << " @" << fps << "fps, " << bitrate << "bps" << std::endl;
    std::cout << "   CTU Size: " << ctu_size_ 
              << ", MTT Depth: " << max_mtt_depth_ 
              << ", Complexity: " << complexity_level_ << std::endl;
    std::cout << "   Advanced Tools: MIP=" << features_.mip_enabled
              << ", Affine=" << features_.affine_enabled
              << ", IBC=" << features_.ibc_enabled << std::endl;
    
    return true;
}

bool VVCEncoder::encode_frame(const VideoFrame& input, std::vector<uint8_t>& output) {
    utils::BitstreamWriter writer;
    
    try {
        if (!encode_vvc_nal_units(input, writer)) {
            return false;
        }
        
        output = writer.get_data();
        frame_count_++;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "VVC Encoding error: " << e.what() << std::endl;
        return false;
    }
}

bool VVCEncoder::encode_vvc_nal_units(const VideoFrame& frame, utils::BitstreamWriter& writer) {
    bool is_idr = (frame_count_ % gop_size_ == 0);
    
    // VVC NAL unit structure
    if (frame_count_ == 0) {
        encode_sps(writer); // Sequence Parameter Set
        encode_pps(writer); // Picture Parameter Set
    }
    
    // AUD (Access Unit Delimiter)
    writer.write_bits(0x00000001, 32); // Start code
    writer.write_bits(0x20, 8); // AUD NAL unit type
    
    // Slice NAL unit
    writer.write_bits(0x00000001, 32); // Start code
    writer.write_bit(0); // forbidden_zero_bit
    writer.write_bits(0, 6); // nal_unit_type
    writer.write_bits(is_idr ? 2 : 0, 6); // VVC NAL unit type
    writer.write_bits(0, 6); // nuh_temporal_id_plus1
    
    encode_slice_header(writer, is_idr);
    
    // Encode CTUs in parallel if enabled
    int ctus_width = (width_ + ctu_size_ - 1) / ctu_size_;
    int ctus_height = (height_ + ctu_size_ - 1) / ctu_size_;
    
    if (parallel_processing_ && complexity_level_ > 3) {
        std::vector<std::future<void>> futures;
        
        for (int y = 0; y < ctus_height; ++y) {
            for (int x = 0; x < ctus_width; ++x) {
                futures.push_back(std::async(std::launch::async, [&, x, y]() {
                    VVCCTU ctu;
                    // Process CTU in parallel
                    encode_ctu(writer, ctu, x * ctu_size_, y * ctu_size_);
                }));
            }
        }
        
        // Wait for all CTUs to complete
        for (auto& future : futures) {
            future.get();
        }
    } else {
        // Sequential processing
        for (int y = 0; y < ctus_height; ++y) {
            for (int x = 0; x < ctus_width; ++x) {
                VVCCTU ctu;
                encode_ctu(writer, ctu, x * ctu_size_, y * ctu_size_);
            }
        }
    }
    
    return true;
}

void VVCEncoder::encode_sps(utils::BitstreamWriter& writer) {
    writer.write_bits(0x00000001, 32); // Start code
    writer.write_bits(0x21, 8); // SPS NAL unit type
    
    // SPS content
    writer.write_ue(0); // sps_seq_parameter_set_id
    writer.write_ue(1); // vvc_parameter_set_id
    
    // Profile, tier, level
    writer.write_bits(0x11, 8); // general_profile_idc (Main 10)
    writer.write_bit(0); // general_tier_flag
    
    // Video parameters
    writer.write_ue(width_ - 1);
    writer.write_ue(height_ - 1);
    writer.write_bit(0); // separate_colour_plane_flag
    
    // VVC specific parameters
    writer.write_ue(ctu_size_ == 256 ? 3 : (ctu_size_ == 128 ? 2 : 1)); // log2_ctu_size_minus5
    writer.write_bit(features_.mip_enabled ? 1 : 0); // mip_flag
    writer.write_bit(features_.affine_enabled ? 1 : 0); // affine_flag
}

void VVCEncoder::encode_ctu(utils::BitstreamWriter& writer, const VVCCTU& ctu, int x, int y) {
    VVCCodingUnit root_cu;
    root_cu.x = x;
    root_cu.y = y;
    root_cu.width = ctu_size_;
    root_cu.height = ctu_size_;
    
    double best_cost = std::numeric_limits<double>::max();
    mtt_partition_decision(root_cu, best_cost);
    
    encode_mtt_structure(writer, root_cu);
}

void VVCEncoder::mtt_partition_decision(VVCCodingUnit& cu, double& best_cost) {
    // VVC'nin Multi-Type Tree partitioning kararı
    std::vector<VVCPartitionType> candidates;
    
    // Complexity-based candidate selection
    if (complexity_level_ >= 8) {
        // Full MTT: Quadtree + Binary Tree + Ternary Tree
        candidates = {
            VVCPartitionType::NO_SPLIT,
            VVCPartitionType::QT_SPLIT,
            VVCPartitionType::BT_HORZ_SPLIT,
            VVCPartitionType::BT_VERT_SPLIT,
            VVCPartitionType::TT_HORZ_SPLIT,
            VVCPartitionType::TT_VERT_SPLIT
        };
    } else if (complexity_level_ >= 5) {
        // Medium: QT + BT
        candidates = {
            VVCPartitionType::NO_SPLIT,
            VVCPartitionType::QT_SPLIT,
            VVCPartitionType::BT_HORZ_SPLIT,
            VVCPartitionType::BT_VERT_SPLIT
        };
    } else {
        // Simple: QT only
        candidates = {
            VVCPartitionType::NO_SPLIT,
            VVCPartitionType::QT_SPLIT
        };
    }
    
    VVCPartitionType best_partition = VVCPartitionType::NO_SPLIT;
    best_cost = std::numeric_limits<double>::max();
    
    for (auto partition : candidates) {
        double cost = evaluate_mtt_partition_cost(cu, partition);
        if (cost < best_cost) {
            best_cost = cost;
            best_partition = partition;
        }
    }
    
    cu.partition_type = best_partition;
    
    // Recursive partitioning for QT/BT/TT
    if (cu.partition_type != VVCPartitionType::NO_SPLIT) {
        int num_parts = 2; // BT/TT
        if (cu.partition_type == VVCPartitionType::QT_SPLIT) {
            num_parts = 4;
        } else if (cu.partition_type == VVCPartitionType::TT_HORZ_SPLIT || 
                  cu.partition_type == VVCPartitionType::TT_VERT_SPLIT) {
            num_parts = 3;
        }
        
        for (int i = 0; i < num_parts; ++i) {
            VVCCodingUnit child_cu;
            setup_child_cu(cu, child_cu, i);
            
            double child_cost;
            mtt_partition_decision(child_cu, child_cost);
            best_cost += child_cost;
        }
    }
}

void VVCEncoder::encode_mtt_structure(utils::BitstreamWriter& writer, const VVCCodingUnit& cu) {
    processing::VVCCABACEncoder cabac;
    
    // Encode partition type
    cabac.encode_mtt_split(writer, cu.partition_type);
    
    if (cu.partition_type == VVCPartitionType::NO_SPLIT) {
        // Encode this CU
        cabac.encode_pred_mode(writer, cu.pred_mode);
        
        // VVC advanced features
        if (features_.mip_enabled) {
            cabac.encode_mip_flag(writer, cu.use_mip);
        }
        if (features_.affine_enabled) {
            cabac.encode_affine_flag(writer, cu.use_affine);
        }
        if (features_.ibc_enabled) {
            cabac.encode_ibc_flag(writer, cu.use_ibc);
        }
        
        // Encode prediction data
        if (cu.use_affine) {
            encode_affine_motion(writer, cu);
        }
        if (cu.use_gpm) {
            encode_geometric_partition(writer, cu);
        }
        
        // Encode transform data
        encode_transform_info(writer, cu.transform);
    } else {
        // Recursively encode child CUs
        int num_children = get_num_children(cu.partition_type);
        for (int i = 0; i < num_children; ++i) {
            VVCCodingUnit child_cu;
            setup_child_cu(cu, child_cu, i);
            encode_mtt_structure(writer, child_cu);
        }
    }
}

void VVCEncoder::encode_affine_motion(utils::BitstreamWriter& writer, const VVCCodingUnit& cu) {
    // VVC Affine motion prediction - 4/6 parameter model
    writer.write_bit(0); // affine_type (4-param or 6-param)
    
    // Control point motion vectors
    for (int i = 0; i < 3; ++i) { // Up to 3 control points
        writer.write_se(0); // mv_diff_x (simplified)
        writer.write_se(0); // mv_diff_y
    }
}

void VVCEncoder::apply_matrix_intra_prediction(VVCCodingUnit& cu) {
    if (!features_.mip_enabled || !cu.use_mip) return;
    
    // VVC Matrix-based Intra Prediction
    // Reduced reference samples + matrix multiplication
    // Daha az bellek kullanımı, daha hızlı prediction
}

void VVCEncoder::apply_intra_block_copy(VVCCodingUnit& cu) {
    if (!features_.ibc_enabled || !cu.use_ibc) return;
    
    // VVC Intra Block Copy - current frame içinde block copy
    // Screen content için çok verimli
    // Hareketli UI'lar, text, graphics için ideal
}

void VVCEncoder::enable_advanced_tools(const VVCAdvancedFeatures& features) {
    features_ = features;
    
    // Complexity-based auto-disable
    if (complexity_level_ < 3) {
        features_.affine_enabled = false;
        features_.mip_enabled = false;
    }
    if (complexity_level_ < 5) {
        features_.ibc_enabled = false;
        features_.palette_enabled = false;
    }
}

void VVCEncoder::set_bitrate(uint32_t bitrate) {
    bitrate_ = bitrate;
    target_bits_per_frame_ = bitrate / fps_;
    
    // VVC QP adjustment - daha agresif bitrate control
    current_qp_ = 35 + (bitrate < 1500000 ? 10 : 0) - (bitrate > 3000000 ? 10 : 0);
    current_qp_ = std::max(15, std::min(55, current_qp_));
}

// Yardımcı fonksiyonlar
double VVCEncoder::evaluate_mtt_partition_cost(const VVCCodingUnit& cu, VVCPartitionType partition) {
    double distortion = calculate_vvc_distortion(cu);
    double rate = calculate_mtt_partition_rate(cu, partition);
    
    // VVC-specific lambda
    double lambda = 0.57 * std::pow(2.0, (current_qp_ - 12) / 3.0);
    
    return distortion + lambda * rate;
}

void VVCEncoder::setup_child_cu(const VVCCodingUnit& parent, VVCCodingUnit& child, int index) {
    child.x = parent.x;
    child.y = parent.y;
    
    switch (parent.partition_type) {
        case VVCPartitionType::QT_SPLIT:
            child.width = parent.width / 2;
            child.height = parent.height / 2;
            child.x += (index % 2) * child.width;
            child.y += (index / 2) * child.height;
            break;
            
        case VVCPartitionType::BT_HORZ_SPLIT:
            child.width = parent.width;
            child.height = parent.height / 2;
            child.y += index * child.height;
            break;
            
        case VVCPartitionType::TT_HORZ_SPLIT:
            child.width = parent.width;
            if (index == 0 || index == 2) {
                child.height = parent.height / 4;
            } else {
                child.height = parent.height / 2;
            }
            // Complex positioning for TT...
            break;
            
        default:
            child = parent;
            break;
    }
}

int VVCEncoder::get_num_children(VVCPartitionType partition) {
    switch (partition) {
        case VVCPartitionType::QT_SPLIT: return 4;
        case VVCPartitionType::BT_HORZ_SPLIT: return 2;
        case VVCPartitionType::BT_VERT_SPLIT: return 2;
        case VVCPartitionType::TT_HORZ_SPLIT: return 3;
        case VVCPartitionType::TT_VERT_SPLIT: return 3;
        default: return 1;
    }
}

} // namespace codec
} // namespace streaming
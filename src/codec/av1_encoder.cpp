// src/codec/av1_encoder.cpp
#include "streaming/codec/av1_encoder.hpp"
#include "streaming/processing/av1_entropy.hpp"
#include "streaming/processing/motion_estimation.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace streaming {
namespace codec {

AV1Encoder::AV1Encoder() 
    : dct_(std::make_unique<processing::DCT>())
    , quantizer_(std::make_unique<processing::Quantizer>()) {}

bool AV1Encoder::initialize(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate) {
    width_ = width;
    height_ = height;
    fps_ = fps;
    bitrate_ = bitrate;
    frame_count_ = 0;
    
    // AV1-specific initialization
    superblock_size_ = 128; // AV1 allows 64x64 or 128x128 superblocks
    current_qp_ = 50; // AV1 QP range: 0-63
    
    target_bits_per_frame_ = bitrate / fps;
    
    // Initialize reference frames buffer
    reference_frames_.resize(width * height * 3 / 2);
    
    std::cout << "🚀 AV1Encoder initialized: " << width << "x" << height 
              << " @" << fps << "fps, " << bitrate << "bps" << std::endl;
    std::cout << "   SuperBlock Size: " << superblock_size_ 
              << ", Speed Preset: " << speed_preset_ << std::endl;
    std::cout << "   Tools - OBMC: " << enable_obmc_ 
              << ", CFL: " << enable_cfl_ 
              << ", Palette: " << enable_palette_ << std::endl;
    
    return true;
}

bool AV1Encoder::encode_frame(const VideoFrame& input, std::vector<uint8_t>& output) {
    utils::BitstreamWriter writer;
    
    try {
        // AV1 uses Open Bitstream Units (OBU) instead of NAL units
        if (!encode_obu_sequence(input, writer)) {
            return false;
        }
        
        output = writer.get_data();
        frame_count_++;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "AV1 Encoding error: " << e.what() << std::endl;
        return false;
    }
}

bool AV1Encoder::encode_obu_sequence(const VideoFrame& frame, utils::BitstreamWriter& writer) {
    bool is_keyframe = (frame_count_ % gop_size_ == 0);
    
    // Temporal Delimiter OBU (optional but recommended)
    writer.write_bits(0b10000, 5); // OBU header: type=TD, extension=0
    writer.write_bit(0); // obu_has_size_field
    writer.write_bit(1); // obu_extension_flag (temporal_id)
    writer.write_bits(0, 3); // temporal_id
    writer.write_bits(0, 2); // spatial_id
    
    // Sequence Header OBU (first frame only)
    if (frame_count_ == 0) {
        writer.write_bits(0b10001, 5); // OBU type=Sequence Header
        writer.write_bit(1); // obu_has_size_field
        
        // Simplified sequence header
        writer.write_bit(1); // still_picture
        writer.write_bit(0); // reduced_still_picture_header
        writer.write_bits(8, 5); // seq_profile (Main profile)
        writer.write_bit(1); // initial_display_delay_present_flag
    }
    
    // Frame Header OBU
    encode_frame_header(writer, is_keyframe);
    
    // Tile Group OBU (actual frame data)
    encode_tile_group(writer, frame);
    
    // Metadata OBUs (optional)
    if (enable_obmc_ || enable_cfl_) {
        writer.write_bits(0b10010, 5); // OBU type=Metadata
        // Encode tool metadata...
    }
    
    return true;
}

void AV1Encoder::encode_frame_header(utils::BitstreamWriter& writer, bool is_keyframe) {
    writer.write_bits(0b10011, 5); // OBU type=Frame Header
    writer.write_bit(1); // obu_has_size_field
    
    writer.write_bit(is_keyframe ? 1 : 0); // show_existing_frame
    if (is_keyframe) {
        writer.write_bit(1); // show_frame
        writer.write_bit(1); // error_resilient_mode
    } else {
        writer.write_bit(0); // show_frame
        writer.write_bit(0); // error_resilient_mode
    }
    
    // Frame size
    writer.write_bit(0); // frame_size_override_flag
    writer.write_ue(width_ - 1);
    writer.write_ue(height_ - 1);
    
    // Render size (same as frame size)
    writer.write_bit(0); // render_size_present_flag
    
    // Frame reference mode
    if (!is_keyframe) {
        writer.write_bit(0); // primary_ref_frame
    }
    
    // Refresh frame flags
    writer.write_bits(0b1, 8); // refresh_frame_flags
    
    // Quantization parameters
    writer.write_ue(current_qp_); // base_q_idx
    
    // AV1 specific tools
    writer.write_bit(enable_obmc_ ? 1 : 0); // enable_obmc
    writer.write_bit(enable_cfl_ ? 1 : 0); // enable_cfl
}

void AV1Encoder::encode_tile_group(utils::BitstreamWriter& writer, const VideoFrame& frame) {
    writer.write_bits(0b10100, 5); // OBU type=Tile Group
    writer.write_bit(1); // obu_has_size_field
    
    // Tile group info (simplified - single tile)
    writer.write_ue(0); // tile_start
    writer.write_ue(0); // tile_size
    
    processing::AV1EntropyEncoder entropy_encoder;
    entropy_encoder.init_frame();
    
    // Encode superblocks
    int sb_width = (width_ + superblock_size_ - 1) / superblock_size_;
    int sb_height = (height_ + superblock_size_ - 1) / superblock_size_;
    
    for (int y = 0; y < sb_height; ++y) {
        for (int x = 0; x < sb_width; ++x) {
            SuperBlock sb;
            // Extract superblock data (simplified)
            encode_superblock(writer, sb, x * superblock_size_, y * superblock_size_);
        }
    }
}

void AV1Encoder::encode_superblock(utils::BitstreamWriter& writer, const SuperBlock& sb, int x, int y) {
    EncodingBlock root_block;
    root_block.x = x;
    root_block.y = y;
    root_block.width = superblock_size_;
    root_block.height = superblock_size_;
    
    double best_cost = std::numeric_limits<double>::max();
    rdo_partition_decision(root_block, best_cost);
    
    // Recursive partition encoding
    encode_partition_tree(writer, root_block);
}

void AV1Encoder::rdo_partition_decision(EncodingBlock& block, double& best_cost) {
    if (speed_preset_ > 6) {
        // Fast mode: minimal partitioning
        block.partition = PartitionType::PARTITION_NONE;
        return;
    }
    
    // AV1'in karmaşık RDO partition kararı
    std::vector<PartitionType> candidates;
    
    // Hız preset'ine göre candidate'ları sınırla
    if (speed_preset_ <= 3) {
        // Best quality: tüm partition tipleri
        candidates = {
            PartitionType::PARTITION_NONE,
            PartitionType::PARTITION_HORZ,
            PartitionType::PARTITION_VERT,
            PartitionType::PARTITION_SPLIT
        };
    } else {
        // Faster: sadece temel partition'lar
        candidates = {
            PartitionType::PARTITION_NONE,
            PartitionType::PARTITION_SPLIT
        };
    }
    
    PartitionType best_partition = PartitionType::PARTITION_NONE;
    best_cost = std::numeric_limits<double>::max();
    
    for (auto partition : candidates) {
        double cost = evaluate_partition_cost(block, partition);
        if (cost < best_cost) {
            best_cost = cost;
            best_partition = partition;
        }
    }
    
    block.partition = best_partition;
    
    // Recursive partitioning
    if (block.partition == PartitionType::PARTITION_SPLIT && 
        block.width > 8 && block.height > 8) {
        // 4-way split için child block'ları değerlendir
        int child_size = block.width / 2;
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                EncodingBlock child;
                child.x = block.x + i * child_size;
                child.y = block.y + j * child_size;
                child.width = child_size;
                child.height = child_size;
                
                double child_cost;
                rdo_partition_decision(child, child_cost);
                best_cost += child_cost;
            }
        }
    }
}

double AV1Encoder::evaluate_partition_cost(const EncodingBlock& block, PartitionType partition) {
    // Rate-Distortion optimization cost calculation
    double distortion = calculate_distortion(block);
    double rate = calculate_partition_rate(block, partition);
    
    // AV1-specific lambda calculation
    double lambda = 0.68 * std::pow(2.0, (current_qp_ - 12) / 3.0);
    
    return distortion + lambda * rate;
}

void AV1Encoder::encode_partition_tree(utils::BitstreamWriter& writer, const EncodingBlock& block) {
    processing::AV1EntropyEncoder entropy_encoder;
    entropy_encoder.encode_partition_type(writer, block.partition);
    
    if (block.partition == PartitionType::PARTITION_NONE) {
        // Encode this block without partitioning
        encode_prediction_mode(writer, block);
        encode_transform_info(writer, block.transform);
        
        if (block.use_palette) {
            encode_palette_mode(writer, block);
        }
    } else {
        // Recursively encode partitions
        int num_parts = 2; // Default for HORZ/VERT
        if (block.partition == PartitionType::PARTITION_SPLIT) {
            num_parts = 4;
        }
        
        int part_width = block.width / ((block.partition == PartitionType::PARTITION_VERT) ? 2 : 1);
        int part_height = block.height / ((block.partition == PartitionType::PARTITION_HORZ) ? 2 : 1);
        
        for (int i = 0; i < num_parts; ++i) {
            EncodingBlock child;
            child.x = block.x + (i % 2) * part_width;
            child.y = block.y + (i / 2) * part_height;
            child.width = part_width;
            child.height = part_height;
            
            // Simplified: assume same partition decision
            child.partition = PartitionType::PARTITION_NONE;
            encode_partition_tree(writer, child);
        }
    }
}

void AV1Encoder::encode_prediction_mode(utils::BitstreamWriter& writer, const EncodingBlock& block) {
    processing::AV1EntropyEncoder entropy_encoder;
    entropy_encoder.encode_prediction_mode(writer, block.pred_mode);
    
    // AV1'in gelişmiş prediction modları için ek bilgiler
    if (block.pred_mode >= PredictionMode::NEARESTMV) {
        // Motion vector encoding
        // Burada gerçek motion vector encoding yapılır
    }
}

void AV1Encoder::apply_obmc_prediction(EncodingBlock& block) {
    if (!enable_obmc_) return;
    
    // Overlapped Block Motion Compensation
    // Komşu bloklardan overlap bölgeleri için prediction yap
    // Bu, block sınırlarında daha smooth geçişler sağlar
}

void AV1Encoder::apply_cfl_prediction(EncodingBlock& block) {
    if (!enable_cfl_) return;
    
    // Chroma from Luma prediction
    // Luma değerlerinden chroma prediction yap
    // Chroma component için bitrate tasarrufu sağlar
}

void AV1Encoder::enable_tools(bool obmc, bool cfl, bool palette, bool warp_motion) {
    enable_obmc_ = obmc && (speed_preset_ <= 6);
    enable_cfl_ = cfl && (speed_preset_ <= 6);
    enable_palette_ = palette && (speed_preset_ <= 4);
    enable_warp_motion_ = warp_motion && (speed_preset_ <= 2);
}

void AV1Encoder::set_speed_preset(int speed) {
    speed_preset_ = std::max(0, std::min(9, speed));
    
    // Speed preset'e göre tool'ları ayarla
    enable_obmc_ = (speed_preset_ <= 6);
    enable_cfl_ = (speed_preset_ <= 6);
    enable_palette_ = (speed_preset_ <= 4);
    enable_warp_motion_ = (speed_preset_ <= 2);
}

void AV1Encoder::set_bitrate(uint32_t bitrate) {
    bitrate_ = bitrate;
    target_bits_per_frame_ = bitrate / fps_;
    
    // AV1 QP adjustment
    current_qp_ = 50 + (bitrate < 2000000 ? 8 : 0) - (bitrate > 4000000 ? 8 : 0);
    current_qp_ = std::max(20, std::min(63, current_qp_));
}

uint32_t AV1Encoder::get_encoded_size() const {
    return frame_count_ * target_bits_per_frame_ / 8;
}

// Yardımcı fonksiyonlar
double AV1Encoder::calculate_distortion(const EncodingBlock& block) {
    // Simplified distortion calculation
    // Gerçek implementasyonda MSE/SSIM hesaplanır
    return block.width * block.height * 10.0;
}

double AV1Encoder::calculate_partition_rate(const EncodingBlock& block, PartitionType partition) {
    // Partition type'ın bit maliyeti
    switch (partition) {
        case PartitionType::PARTITION_NONE: return 1.0;
        case PartitionType::PARTITION_HORZ: return 2.0;
        case PartitionType::PARTITION_VERT: return 2.0;
        case PartitionType::PARTITION_SPLIT: return 3.0;
        default: return 5.0;
    }
}

} // namespace codec
} // namespace streaming
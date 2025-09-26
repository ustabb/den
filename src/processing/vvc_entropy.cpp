// src/processing/vvc_entropy.cpp
#include "streaming/processing/vvc_entropy.hpp"

namespace streaming {
namespace processing {

VVCCABACEncoder::VVCCABACEncoder() {
    // Initialize VVC context models
    mtt_split_ctx_.resize(6);
    pred_mode_ctx_.resize(10);
    affine_flag_ctx_.resize(2);
    mip_flag_ctx_.resize(2);
    
    // Initialize with default probabilities
    for (auto& ctx : mtt_split_ctx_) {
        ctx.state = 63;
        ctx.mps = 0;
        ctx.count = 0;
    }
}

void VVCCABACEncoder::encode_mtt_split(utils::BitstreamWriter& writer, VVCPartitionType split_type) {
    uint32_t symbol = static_cast<uint32_t>(split_type);
    
    // Complex MTT split encoding with context modeling
    if (symbol == 0) { // NO_SPLIT
        encode_bin(writer, 0, mtt_split_ctx_[0]);
    } else {
        encode_bin(writer, 1, mtt_split_ctx_[0]);
        
        if (symbol == 1) { // QT_SPLIT
            encode_bin(writer, 0, mtt_split_ctx_[1]);
        } else {
            encode_bin(writer, 1, mtt_split_ctx_[1]);
            
            // Encode BT/TT types...
        }
    }
}

void VVCCABACEncoder::encode_bin(utils::BitstreamWriter& writer, uint32_t bin, VVCContextModel& ctx) {
    // VVC CABAC encoding - daha gelişmiş context modeling
    uint32_t range = ctx.state * 4;
    uint32_t offset = 0;
    
    if (bin == ctx.mps) {
        // Encode MPS
        offset = range;
        // Update state for MPS
        if (ctx.state > 0) ctx.state--;
    } else {
        // Encode LPS
        offset = 1024 - range; // Range is 10-bit in VVC
        // Update state for LPS
        ctx.state = (ctx.state < 63) ? ctx.state + 1 : 63;
        ctx.mps = 1 - ctx.mps; // Toggle MPS if needed
    }
    
    writer.write_bits(offset, 10);
}

} // namespace processing
} // namespace streaming
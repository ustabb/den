// include/streaming/processing/vvc_entropy.hpp
#pragma once

#include <vector>
#include <cstdint>

namespace streaming {
namespace processing {

class VVCCABACEncoder {
private:
    struct VVCContextModel {
        uint8_t state;
        uint8_t mps;
        uint16_t count;
    };

public:
    VVCCABACEncoder();
    
    void init_vvc_slice();
    void encode_mtt_split(utils::BitstreamWriter& writer, VVCPartitionType split_type);
    void encode_pred_mode(utils::BitstreamWriter& writer, VVCPredictionMode mode);
    void encode_affine_flag(utils::BitstreamWriter& writer, bool is_affine);
    void encode_mip_flag(utils::BitstreamWriter& writer, bool use_mip);
    
    // VVC-specific advanced encoding
    void encode_ibc_flag(utils::BitstreamWriter& writer, bool use_ibc);
    void encode_gpm_info(utils::BitstreamWriter& writer, int partition_idx, int angle);
    void encode_bdpcm_dir(utils::BitstreamWriter& writer, int direction);

private:
    void encode_bin(utils::BitstreamWriter& writer, uint32_t bin, VVCContextModel& ctx);
    void encode_bin_ep(utils::BitstreamWriter& writer, uint32_t bin); // Equal probability
    void encode_bin_tr(utils::BitstreamWriter& writer, uint32_t bin); // Bypass
    
    // VVC context models
    std::vector<VVCContextModel> mtt_split_ctx_;
    std::vector<VVCContextModel> pred_mode_ctx_;
    std::vector<VVCContextModel> affine_flag_ctx_;
    std::vector<VVCContextModel> mip_flag_ctx_;
};

} // namespace processing
} // namespace streaming
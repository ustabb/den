// include/streaming/codec/vvc_encoder.hpp
#pragma once

#include "video_codec.hpp"
#include "vvc_structures.hpp"
#include "../utils/bitstream.hpp"
#include "../processing/dct_transform.hpp"
#include "../processing/quantization.hpp"
#include <memory>
#include <vector>

namespace streaming {
namespace codec {

class VVCEncoder : public IVideoEncoder {
private:
    struct VVCCodingUnit {
        int x, y;
        int width, height;
        VVCPartitionType partition_type;
        VVCPredictionMode pred_mode;
        VVCTransformUnit transform;
        
        // VVC-specific advanced features
        bool use_mip = false;        // Matrix-based Intra Prediction
        bool use_affine = false;     // Affine motion
        bool use_ibc = false;        // Intra Block Copy
        bool use_gpm = false;        // Geometric Partition Mode
        bool use_bdpcm = false;      // Block DPCM
    };

public:
    VVCEncoder();
    ~VVCEncoder() override = default;
    
    bool initialize(uint32_t width, uint32_t height, uint32_t fps, 
                   uint32_t bitrate) override;
    bool encode_frame(const VideoFrame& input, std::vector<uint8_t>& output) override;
    void set_bitrate(uint32_t bitrate) override;
    void set_gop_size(uint32_t gop_size) override;
    uint32_t get_encoded_size() const override;

    // VVC-specific advanced features
    void enable_advanced_tools(const VVCAdvancedFeatures& features);
    void set_complexity_level(int level); // 0=simple, 10=full
    void set_parallel_processing(bool enabled);

private:
    bool encode_vvc_nal_units(const VideoFrame& frame, utils::BitstreamWriter& writer);
    void encode_sps(utils::BitstreamWriter& writer); // Sequence Parameter Set
    void encode_pps(utils::BitstreamWriter& writer); // Picture Parameter Set
    void encode_slice_header(utils::BitstreamWriter& writer, bool is_idr);
    void encode_ctu(utils::BitstreamWriter& writer, const VVCCTU& ctu, int x, int y);
    
    // VVC'nin yeni algoritmaları
    void mtt_partition_decision(VVCCodingUnit& cu, double& best_cost);
    void encode_mtt_structure(utils::BitstreamWriter& writer, const VVCCodingUnit& cu);
    void encode_affine_motion(utils::BitstreamWriter& writer, const VVCCodingUnit& cu);
    void encode_geometric_partition(utils::BitstreamWriter& writer, const VVCCodingUnit& cu);
    
    // Yeni VVC teknolojileri
    void apply_matrix_intra_prediction(VVCCodingUnit& cu);
    void apply_intra_block_copy(VVCCodingUnit& cu);
    void apply_bdpcm_coding(VVCCodingUnit& cu);
    void apply_multi_transform_selection(VVCTransformUnit& tu);

private:
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t fps_ = 30;
    uint32_t bitrate_ = 1000000;
    uint32_t gop_size_ = 32;
    uint32_t frame_count_ = 0;
    
    int ctu_size_ = 128;        // VVC: 128x128 default (256x256'a kadar)
    int max_mtt_depth_ = 4;     // Multi-Type Tree max depth
    int current_qp_ = 35;       // VVC QP range: 0-63
    int complexity_level_ = 5;  // Complexity setting
    
    VVCAdvancedFeatures features_;
    bool parallel_processing_ = true;
    
    std::unique_ptr<processing::DCT> dct_;
    std::unique_ptr<processing::Quantizer> quantizer_;
    std::vector<uint8_t> reference_frames_;
    
    // VVC için yeni buffer'lar
    std::vector<uint8_t> ibc_buffer_; // Intra Block Copy buffer
};

} // namespace codec
} // namespace streaming
// include/streaming/codec/av1_encoder.hpp
#pragma once

#include "video_codec.hpp"
#include "av1_structures.hpp"
#include "../utils/bitstream.hpp"
#include "../processing/dct_transform.hpp"
#include "../processing/quantization.hpp"
#include <memory>
#include <vector>

namespace streaming {
namespace codec {

class AV1Encoder : public IVideoEncoder {
private:
    struct EncodingBlock {
        int x, y;
        int width, height;
        PartitionType partition;
        PredictionMode pred_mode;
        TransformBlock transform;
        
        // AV1-specific features
        bool use_palette = false;
        bool use_obmc = false;  // Overlapped Block Motion Compensation
        bool use_cfl = false;   // Chroma from Luma
    };

public:
    AV1Encoder();
    ~AV1Encoder() override = default;
    
    bool initialize(uint32_t width, uint32_t height, uint32_t fps, 
                   uint32_t bitrate) override;
    bool encode_frame(const VideoFrame& input, std::vector<uint8_t>& output) override;
    void set_bitrate(uint32_t bitrate) override;
    void set_gop_size(uint32_t gop_size) override;
    uint32_t get_encoded_size() const override;

    // AV1-specific optimizations
    void enable_tools(bool obmc, bool cfl, bool palette, bool warp_motion);
    void set_speed_preset(int speed);  // 0=best quality, 9=fastest

private:
    bool encode_obu_sequence(const VideoFrame& frame, utils::BitstreamWriter& writer);
    void encode_frame_header(utils::BitstreamWriter& writer, bool is_keyframe);
    void encode_tile_group(utils::BitstreamWriter& writer, const VideoFrame& frame);
    void encode_superblock(utils::BitstreamWriter& writer, const SuperBlock& sb, int x, int y);
    
    // AV1'in benzersiz özellikleri
    void rdo_partition_decision(EncodingBlock& block, double& best_cost);
    void encode_prediction_mode(utils::BitstreamWriter& writer, const EncodingBlock& block);
    void encode_transform_info(utils::BitstreamWriter& writer, const TransformBlock& tx);
    void encode_palette_mode(utils::BitstreamWriter& writer, const EncodingBlock& block);
    
    // Yeni AV1 teknolojileri
    void apply_obmc_prediction(EncodingBlock& block);
    void apply_cfl_prediction(EncodingBlock& block);
    void apply_warped_motion_compensation(EncodingBlock& block);

private:
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t fps_ = 30;
    uint32_t bitrate_ = 1000000;
    uint32_t gop_size_ = 30;
    uint32_t frame_count_ = 0;
    
    int superblock_size_ = 128;  // AV1'in büyük blokları
    int current_qp_ = 50;        // AV1 QP range: 0-63
    int speed_preset_ = 5;       // Balanced preset
    
    // AV1 tool flags
    bool enable_obmc_ = true;
    bool enable_cfl_ = true;
    bool enable_palette_ = true;
    bool enable_warp_motion_ = true;
    
    std::unique_ptr<processing::DCT> dct_;
    std::unique_ptr<processing::Quantizer> quantizer_;
    std::vector<uint8_t> reference_frames_;
};

} // namespace codec
} // namespace streaming
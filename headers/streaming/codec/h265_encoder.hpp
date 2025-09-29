// include/streaming/codec/h265_encoder.hpp
#pragma once

#include "video_codec.hpp"
#include "hevc_structures.hpp"
#include "../utils/bitstream.hpp"
#include "../processing/dct_transform.hpp"
#include "../processing/quantization.hpp"
#include <memory>
#include <vector>

namespace streaming {
namespace codec {

class H265Encoder : public IVideoEncoder {
private:
    struct CodingUnit {
        int x, y; // Position
        int size; // CU size (64,32,16,8)
        bool split; // Whether to split into smaller CUs
        
        // Prediction and transform info
        PredictionUnit pu;
        TransformUnit tu;
    };

public:
    H265Encoder();
    ~H265Encoder() override = default;
    
    bool initialize(uint32_t width, uint32_t height, uint32_t fps, 
                   uint32_t bitrate) override;
    bool encode_frame(const VideoFrame& input, std::vector<uint8_t>& output) override;
    void set_bitrate(uint32_t bitrate) override;
    void set_gop_size(uint32_t gop_size) override;
    uint32_t get_encoded_size() const override;

private:
    bool encode_nal_unit(const VideoFrame& frame, utils::BitstreamWriter& writer);
    void encode_slice_header(utils::BitstreamWriter& writer, bool is_idr);
    void encode_ctu(utils::BitstreamWriter& writer, const CTU& ctu, int x, int y);
    void encode_coding_unit(utils::BitstreamWriter& writer, const CodingUnit& cu);
    
    // HEVC-specific encoding tools
    void rdo_ctu_split_decision(CTU& ctu, int x, int y, std::vector<CodingUnit>& cus);
    void encode_intra_prediction(utils::BitstreamWriter& writer, const CodingUnit& cu);
    void encode_inter_prediction(utils::BitstreamWriter& writer, const CodingUnit& cu);
    void encode_residual_quadtree(utils::BitstreamWriter& writer, const TransformUnit& tu);
    
    // New HEVC features
    void encode_sao_parameters(utils::BitstreamWriter& writer); // Sample Adaptive Offset
    void encode_deblocking_params(utils::BitstreamWriter& writer); // Deblocking filter
    
private:
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t fps_ = 30;
    uint32_t bitrate_ = 1000000;
    uint32_t gop_size_ = 30;
    uint32_t frame_count_ = 0;
    
    int ctu_size_ = 64; // HEVC uses larger CTUs (64x64)
    int max_cu_depth_ = 3; // Maximum CU split depth
    int current_qp_ = 32; // HEVC uses different QP range
    
    std::unique_ptr<processing::DCT> dct_;
    std::unique_ptr<processing::Quantizer> quantizer_;
    std::vector<uint8_t> reference_frames_;
};

} // namespace codec
} // namespace streaming
// include/streaming/codec/h264_decoder.hpp
#pragma once

#include "video_codec.hpp"
#include "../utils/bitstream.hpp"
#include "../processing/dct_transform.hpp"
#include "../processing/quantization.hpp"

namespace streaming {
namespace codec {

class H264Decoder : public IVideoDecoder {
public:
    H264Decoder();
    ~H264Decoder() override = default;
    
    bool initialize() override;
    bool decode_frame(const uint8_t* data, size_t size, VideoFrame& output) override;
    void reset() override;

private:
    bool decode_nal_unit(utils::BitstreamReader& reader, VideoFrame& output);
    void decode_slice_data(utils::BitstreamReader& reader, VideoFrame& output);
    void decode_macroblock(utils::BitstreamReader& reader, VideoFrame& output, 
                          uint32_t mb_x, uint32_t mb_y);
    
private:
    std::unique_ptr<processing::DCT> dct_;
    std::unique_ptr<processing::Quantizer> quantizer_;
    std::vector<uint8_t> reference_frame_;
    uint32_t width_ = 0, height_ = 0;
    int current_qp_ = 26;
};

} // namespace codec
} // namespace streaming
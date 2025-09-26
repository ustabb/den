// include/streaming/processing/av1_entropy.hpp
#pragma once

#include <vector>
#include <cstdint>

namespace streaming {
namespace processing {

class AV1EntropyEncoder {
private:
    struct SymbolContext {
        uint16_t cumulative_freq;
        uint16_t symbol_freq;
    };

public:
    AV1EntropyEncoder();
    
    void init_frame();
    void encode_symbol(utils::BitstreamWriter& writer, uint16_t symbol, 
                      const std::vector<SymbolContext>& contexts);
    void encode_coeffs(utils::BitstreamWriter& writer, 
                      const std::vector<std::vector<int16_t>>& coeffs, 
                      int tx_size, bool is_intra);
    
    // AV1-specific encoding functions
    void encode_partition_type(utils::BitstreamWriter& writer, PartitionType partition);
    void encode_prediction_mode(utils::BitstreamWriter& writer, PredictionMode mode);
    void encode_mv_component(utils::BitstreamWriter& writer, int16_t mv_component);

private:
    void encode_cdf(utils::BitstreamWriter& writer, uint16_t symbol, 
                   const uint16_t* cdf, int precision);
    void update_cdf(uint16_t* cdf, uint16_t symbol, int size);
    
    // AV1 adaptive probability models
    std::vector<SymbolContext> partition_cdf_;
    std::vector<SymbolContext> pred_mode_cdf_;
    std::vector<SymbolContext> mv_joint_cdf_;
};

} // namespace processing
} // namespace streaming
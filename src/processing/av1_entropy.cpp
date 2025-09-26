// src/processing/av1_entropy.cpp
#include "streaming/processing/av1_entropy.hpp"

namespace streaming {
namespace processing {

AV1EntropyEncoder::AV1EntropyEncoder() {
    // Initialize CDFs (Cumulative Distribution Functions)
    partition_cdf_.resize(10);
    pred_mode_cdf_.resize(20);
    mv_joint_cdf_.resize(4);
}

void AV1EntropyEncoder::encode_partition_type(utils::BitstreamWriter& writer, PartitionType partition) {
    uint16_t symbol = static_cast<uint16_t>(partition);
    
    // Simplified CDF for partition types
    uint16_t partition_cdf[] = {2048, 1536, 1024, 512, 256, 128, 64, 32, 16, 0};
    encode_cdf(writer, symbol, partition_cdf, 12);
}

void AV1EntropyEncoder::encode_prediction_mode(utils::BitstreamWriter& writer, PredictionMode mode) {
    uint16_t symbol = static_cast<uint16_t>(mode);
    
    // Simplified CDF for prediction modes
    uint16_t mode_cdf[] = {2048, 1920, 1792, 1664, 1536, 1408, 1280, 1152, 
                          1024, 896, 768, 640, 512, 384, 256, 128, 64, 32, 16, 0};
    encode_cdf(writer, symbol, mode_cdf, 12);
}

void AV1EntropyEncoder::encode_cdf(utils::BitstreamWriter& writer, uint16_t symbol, 
                                  const uint16_t* cdf, int precision) {
    uint16_t value = 0;
    uint16_t scale = 1 << precision;
    
    for (uint16_t i = 0; i <= symbol; ++i) {
        value += cdf[i];
    }
    
    // Normalize and encode
    uint32_t normalized = (value * scale) / cdf[0];
    writer.write_bits(normalized, precision);
}

void AV1EntropyEncoder::encode_coeffs(utils::BitstreamWriter& writer, 
                                     const std::vector<std::vector<int16_t>>& coeffs, 
                                     int tx_size, bool is_intra) {
    // AV1'in karmaşık coefficient encoding'i
    // DC coefficient first
    if (!coeffs.empty() && !coeffs[0].empty()) {
        int16_t dc_coeff = coeffs[0][0];
        writer.write_se(dc_coeff);
    }
    
    // AC coefficients with special scanning
    // AV1 uses multiple scan patterns based on transform type
    for (size_t i = 1; i < coeffs.size(); ++i) {
        for (size_t j = 0; j < coeffs[i].size(); ++j) {
            if (coeffs[i][j] != 0) {
                writer.write_se(coeffs[i][j]);
                // Encode sign and magnitude separately
            }
        }
    }
}

} // namespace processing
} // namespace streaming
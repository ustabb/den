#pragma once
#include <vector>
#include <cstdint>

namespace streaming {
namespace audio {

// Simple audio resampler: converts PCM data between sample rates
class Resampler {
public:
    Resampler(int inRate, int outRate);
    std::vector<int16_t> resample(const std::vector<int16_t>& input);
    int getInputRate() const;
    int getOutputRate() const;
private:
    int inputRate_;
    int outputRate_;
};

} // namespace audio
} // namespace streaming

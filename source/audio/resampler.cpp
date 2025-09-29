#include "streaming/audio/resampler.hpp"
#include <vector>
#include <cstdint>

namespace streaming {
namespace audio {

Resampler::Resampler(int inRate, int outRate)
    : inputRate_(inRate), outputRate_(outRate) {}

int Resampler::getInputRate() const { return inputRate_; }
int Resampler::getOutputRate() const { return outputRate_; }

std::vector<int16_t> Resampler::resample(const std::vector<int16_t>& input) {
    if (inputRate_ == outputRate_ || input.empty()) return input;
    std::vector<int16_t> output;
    double ratio = static_cast<double>(outputRate_) / inputRate_;
    size_t outLen = static_cast<size_t>(input.size() * ratio);
    output.reserve(outLen);
    for (size_t i = 0; i < outLen; ++i) {
        size_t idx = static_cast<size_t>(i / ratio);
        if (idx < input.size())
            output.push_back(input[idx]);
        else
            output.push_back(0);
    }
    return output;
}

} // namespace audio
} // namespace streaming

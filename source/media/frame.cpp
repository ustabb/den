#include "media/frame.hpp"

using namespace media;

RawFrame::RawFrame() {}

RawFrame::RawFrame(const std::vector<uint8_t>& d, const FrameMetadata& m) 
    : data(d), meta(m) {}

RawFrame::RawFrame(const std::vector<uint8_t>& d, uint32_t w, uint32_t h, uint32_t c, uint32_t s, const std::string& f)
    : data(d), meta(w, h, c, s, f) {}

RawFrame RawFrame::from_vector(const std::vector<uint8_t>& vec, const FrameMetadata& m) {
    return RawFrame(vec, m);
}

RawFrame RawFrame::from_vector(const std::vector<uint8_t>& vec, uint32_t w, uint32_t h, uint32_t c) {
    return RawFrame(vec, FrameMetadata(w, h, c));
}

std::vector<uint8_t> RawFrame::to_vector() const {
    return data;
}

bool RawFrame::empty() const { 
    return data.empty(); 
}

size_t RawFrame::size() const { 
    return data.size(); 
}
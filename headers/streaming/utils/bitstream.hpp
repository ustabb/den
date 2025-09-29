// include/streaming/utils/bitstream.hpp
#pragma once

#include <cstdint>
#include <vector>
#include <stdexcept>

namespace streaming {
namespace utils {

class BitstreamWriter {
private:
    std::vector<uint8_t> buffer_;
    uint32_t current_byte_ = 0;
    uint8_t current_bit_ = 0;
    
public:
    void write_bit(bool bit) {
        if (current_bit_ == 0) {
            buffer_.push_back(0);
            current_byte_ = buffer_.size() - 1;
        }
        
        if (bit) {
            buffer_[current_byte_] |= (1 << (7 - current_bit_));
        }
        
        current_bit_ = (current_bit_ + 1) % 8;
    }
    
    void write_bits(uint32_t value, uint8_t num_bits) {
        for (int8_t i = num_bits - 1; i >= 0; --i) {
            write_bit((value >> i) & 1);
        }
    }
    
    void write_ue(uint32_t value) { // Exponential Golomb coding
        uint32_t leading_zeros = 0;
        uint32_t temp = value + 1;
        
        while (temp > 1) {
            temp >>= 1;
            leading_zeros++;
        }
        
        write_bits(0, leading_zeros);
        write_bits(value + 1, leading_zeros + 1);
    }
    
    void write_se(int32_t value) { // Signed Exponential Golomb
        if (value <= 0) {
            write_ue(-2 * value);
        } else {
            write_ue(2 * value - 1);
        }
    }
    
    const std::vector<uint8_t>& get_data() const { return buffer_; }
    void clear() { buffer_.clear(); current_byte_ = 0; current_bit_ = 0; }
};

class BitstreamReader {
private:
    const uint8_t* data_;
    size_t size_;
    size_t current_byte_ = 0;
    uint8_t current_bit_ = 0;
    
public:
    BitstreamReader(const uint8_t* data, size_t size) : data_(data), size_(size) {}
    
    bool read_bit() {
        if (current_byte_ >= size_) {
            throw std::runtime_error("Bitstream read overflow");
        }
        
        bool bit = (data_[current_byte_] >> (7 - current_bit_)) & 1;
        current_bit_++;
        
        if (current_bit_ == 8) {
            current_byte_++;
            current_bit_ = 0;
        }
        
        return bit;
    }
    
    uint32_t read_bits(uint8_t num_bits) {
        uint32_t value = 0;
        for (uint8_t i = 0; i < num_bits; ++i) {
            value = (value << 1) | read_bit();
        }
        return value;
    }
    
    uint32_t read_ue() {
        uint32_t leading_zeros = 0;
        while (read_bit() == 0) {
            leading_zeros++;
        }
        
        if (leading_zeros == 0) return 0;
        
        uint32_t value = read_bits(leading_zeros);
        return value + (1 << leading_zeros) - 1;
    }
    
    int32_t read_se() {
        uint32_t ue = read_ue();
        if (ue % 2 == 0) {
            return -static_cast<int32_t>(ue / 2);
        } else {
            return static_cast<int32_t>((ue + 1) / 2);
        }
    }
};

} // namespace utils
} // namespace streaming
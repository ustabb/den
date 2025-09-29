// include/streaming/performance/vectorization.hpp
#pragma once

#include <cstdint>
#include <immintrin.h>  // AVX, AVX2, AVX-512
#include <arm_neon.h>   // ARM NEON

namespace streaming {
namespace performance {

class SIMDVectorizer {
public:
    #ifdef __AVX512F__
    using Float8 = __m512;
    using Int16 = __m512i;
    static constexpr size_t VECTOR_SIZE = 16;
    #elif defined(__AVX2__)
    using Float8 = __m256;
    using Int16 = __m256i;
    static constexpr size_t VECTOR_SIZE = 8;
    #elif defined(__SSE4_1__)
    using Float8 = __m128;
    using Int16 = __m128i;
    static constexpr size_t VECTOR_SIZE = 4;
    #elif defined(__ARM_NEON)
    using Float8 = float32x4_t;
    using Int16 = int32x4_t;
    static constexpr size_t VECTOR_SIZE = 4;
    #else
    static constexpr size_t VECTOR_SIZE = 1;
    #endif

    SIMDVectorizer();
    
    // Vectorized image processing
    void vectorized_convert_yuv_to_rgb(const uint8_t* y_plane, const uint8_t* u_plane, 
                                      const uint8_t* v_plane, uint8_t* rgb_output,
                                      size_t width, size_t height);
    
    void vectorized_scale_image(const uint8_t* input, uint8_t* output,
                               size_t in_width, size_t in_height,
                               size_t out_width, size_t out_height);
    
    // Vectorized DCT/IDCT
    void vectorized_dct_8x8(const int16_t* input, float* output);
    void vectorized_idct_8x8(const float* input, int16_t* output);
    
    // Vectorized motion estimation
    uint32_t vectorized_sad_16x16(const uint8_t* block1, const uint8_t* block2, size_t stride);
    uint32_t vectorized_satd_4x4(const int16_t* block1, const int16_t* block2);
    
    // Vectorized quantization
    void vectorized_quantize_8x8(const float* dct_coeffs, int16_t* quantized, 
                                const float* quantization_table);
    void vectorized_dequantize_8x8(const int16_t* quantized, float* dct_coeffs,
                                  const float* quantization_table);

    // Runtime CPU feature detection
    bool supports_avx512() const;
    bool supports_avx2() const;
    bool supports_sse4() const;
    bool supports_neon() const;

private:
    // AVX implementations
    #ifdef __AVX2__
    void avx2_convert_yuv_to_rgb(const uint8_t* y, const uint8_t* u, const uint8_t* v,
                                uint8_t* rgb, size_t width, size_t height);
    uint32_t avx2_sad_16x16(const uint8_t* block1, const uint8_t* block2, size_t stride);
    #endif
    
    // SSE implementations
    #ifdef __SSE4_1__
    void sse4_convert_yuv_to_rgb(const uint8_t* y, const uint8_t* u, const uint8_t* v,
                                uint8_t* rgb, size_t width, size_t height);
    uint32_t sse4_sad_16x16(const uint8_t* block1, const uint8_t* block2, size_t stride);
    #endif
    
    // NEON implementations
    #ifdef __ARM_NEON
    void neon_convert_yuv_to_rgb(const uint8_t* y, const uint8_t* u, const uint8_t* v,
                                uint8_t* rgb, size_t width, size_t height);
    uint32_t neon_sad_16x16(const uint8_t* block1, const uint8_t* block2, size_t stride);
    #endif
    
    bool avx512_supported_ = false;
    bool avx2_supported_ = false;
    bool sse4_supported_ = false;
    bool neon_supported_ = false;
};

// Template metaprogramming for compile-time vectorization
template<size_t VectorSize>
class VectorizedLoop {
public:
    template<typename Func>
    static void execute(size_t total_elements, Func&& func) {
        constexpr size_t step = VectorSize;
        size_t i = 0;
        
        // Vectorized part
        for (; i + step <= total_elements; i += step) {
            func(i, step);
        }
        
        // Scalar remainder
        for (; i < total_elements; ++i) {
            func(i, 1);
        }
    }
};

} // namespace performance
} // namespace streaming
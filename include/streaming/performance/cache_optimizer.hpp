// include/streaming/performance/cache_optimizer.hpp
#pragma once

#include <cstdint>
#include <vector>
#include <memory>

namespace streaming {
namespace performance {

class CacheOptimizer {
public:
    struct CacheInfo {
        size_t l1_cache_size = 32768;      // 32KB
        size_t l2_cache_size = 262144;     // 256KB  
        size_t l3_cache_size = 8388608;    // 8MB
        size_t cache_line_size = 64;       // 64 bytes
        size_t page_size = 4096;           // 4KB
    };

    struct MemoryAccessPattern {
        enum Pattern {
            SEQUENTIAL,
            RANDOM,
            STRIDED,
            TILED
        };
        
        Pattern pattern;
        size_t stride;
        size_t block_size;
        bool prefetch_friendly;
    };

    CacheOptimizer();
    
    // Memory layout optimization
    template<typename T>
    void optimize_layout(std::vector<T>& data, size_t element_size, MemoryAccessPattern pattern);
    
    // Prefetch optimization
    void prefetch_data(const void* data, size_t size, int locality = 3);
    void optimize_prefetch_pattern(void* data, size_t size, size_t stride);
    
    // Cache-aware algorithms
    void optimize_matrix_multiply(float* A, float* B, float* C, 
                                 size_t m, size_t n, size_t p);
    void optimize_image_processing(uint8_t* image, size_t width, size_t height, 
                                  size_t channels, size_t stride);
    
    // False sharing prevention
    template<typename T>
    class CacheAlignedAllocator {
    public:
        typedef T value_type;
        CacheAlignedAllocator() = default;
        
        template<class U>
        CacheAlignedAllocator(const CacheAlignedAllocator<U>&) {}
        
        T* allocate(size_t n) {
            size_t alignment = cache_info_.cache_line_size;
            size_t size = n * sizeof(T);
            void* ptr = aligned_alloc(alignment, (size + alignment - 1) & ~(alignment - 1));
            if (!ptr) throw std::bad_alloc();
            return static_cast<T*>(ptr);
        }
        
        void deallocate(T* p, size_t n) {
            free(p);
        }
        
    private:
        CacheInfo cache_info_;
    };

private:
    CacheInfo detect_cache_sizes();
    MemoryAccessPattern analyze_access_pattern(const void* data, size_t size, size_t access_size);
    void apply_tiling_optimization(void* data, size_t width, size_t height, size_t elem_size);
    
    CacheInfo cache_info_;
};

// Cache-friendly data structures
template<typename T, size_t Alignment = 64>
class CacheAlignedVector {
public:
    CacheAlignedVector(size_t size) : data_(size) {}
    
    T* data() { return data_.data(); }
    const T* data() const { return data_.data(); }
    size_t size() const { return data_.size(); }
    
    // Ensure alignment
    void resize(size_t new_size) {
        data_.resize((new_size + Alignment - 1) & ~(Alignment - 1));
    }
    
private:
    std::vector<T, CacheOptimizer::CacheAlignedAllocator<T>> data_;
};

} // namespace performance
} // namespace streaming
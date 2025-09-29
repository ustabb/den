// include/streaming/performance/memory_pool.hpp
#pragma once

#include <memory>
#include <vector>
#include <atomic>
#include <new>

namespace streaming {
namespace performance {

template<typename T, size_t PoolSize = 1024>
class LockFreeMemoryPool {
private:
    struct Node {
        std::atomic<Node*> next;
        alignas(64) uint8_t data[sizeof(T)];
    };

    struct alignas(64) PoolBlock {
        Node nodes[PoolSize];
        PoolBlock* next;
    };

public:
    LockFreeMemoryPool() {
        // Initialize first block
        current_block_ = new PoolBlock();
        initialize_block(current_block_);
        
        // Pre-allocate free list
        for (size_t i = 0; i < PoolSize; ++i) {
            Node* node = &current_block_->nodes[i];
            node->next.store(free_list_.load(std::memory_order_relaxed), 
                           std::memory_order_relaxed);
            free_list_.store(node, std::memory_order_relaxed);
        }
    }
    
    ~LockFreeMemoryPool() {
        PoolBlock* block = current_block_;
        while (block) {
            PoolBlock* next = block->next;
            delete block;
            block = next;
        }
    }
    
    template<typename... Args>
    T* construct(Args&&... args) {
        Node* node = allocate_node();
        if (!node) {
            return nullptr; // Pool exhausted
        }
        
        T* obj = new(node->data) T(std::forward<Args>(args)...);
        return obj;
    }
    
    void destroy(T* object) {
        if (!object) return;
        
        object->~T();
        Node* node = reinterpret_cast<Node*>(reinterpret_cast<uint8_t*>(object) - offsetof(Node, data));
        deallocate_node(node);
    }
    
    // Statistics
    size_t get_allocated_count() const { return allocated_count_.load(std::memory_order_relaxed); }
    size_t get_free_count() const { return free_count_.load(std::memory_order_relaxed); }
    size_t get_total_capacity() const { return total_capacity_.load(std::memory_order_relaxed); }

private:
    Node* allocate_node() {
        Node* node = free_list_.load(std::memory_order_acquire);
        
        while (node) {
            if (free_list_.compare_exchange_weak(node, node->next.load(std::memory_order_relaxed),
                                               std::memory_order_acq_rel)) {
                allocated_count_.fetch_add(1, std::memory_order_relaxed);
                free_count_.fetch_sub(1, std::memory_order_relaxed);
                return node;
            }
        }
        
        // Pool exhausted, allocate new block (fallback to new)
        return allocate_new_block();
    }
    
    void deallocate_node(Node* node) {
        if (!node) return;
        
        Node* old_head = free_list_.load(std::memory_order_acquire);
        do {
            node->next.store(old_head, std::memory_order_relaxed);
        } while (!free_list_.compare_exchange_weak(old_head, node,
                                                 std::memory_order_acq_rel,
                                                 std::memory_order_acquire));
        
        allocated_count_.fetch_sub(1, std::memory_order_relaxed);
        free_count_.fetch_add(1, std::memory_order_relaxed);
    }
    
    Node* allocate_new_block() {
        std::lock_guard<std::mutex> lock(block_mutex_);
        
        PoolBlock* new_block = new PoolBlock();
        initialize_block(new_block);
        
        // Add to block list
        new_block->next = current_block_;
        current_block_ = new_block;
        
        total_capacity_.fetch_add(PoolSize, std::memory_order_relaxed);
        free_count_.fetch_add(PoolSize, std::memory_order_relaxed);
        
        return &new_block->nodes[0];
    }
    
    void initialize_block(PoolBlock* block) {
        for (size_t i = 0; i < PoolSize - 1; ++i) {
            block->nodes[i].next.store(&block->nodes[i + 1], std::memory_order_relaxed);
        }
        block->nodes[PoolSize - 1].next.store(nullptr, std::memory_order_relaxed);
    }
    
    std::atomic<Node*> free_list_ = {nullptr};
    std::atomic<size_t> allocated_count_ = {0};
    std::atomic<size_t> free_count_ = {0};
    std::atomic<size_t> total_capacity_ = {0};
    
    PoolBlock* current_block_ = nullptr;
    std::mutex block_mutex_;
};

// Specialized memory pools for common types
using FrameMemoryPool = LockFreeMemoryPool<uint8_t, 8192>; // For video frames
using PacketMemoryPool = LockFreeMemoryPool<uint8_t, 16384>; // For network packets

} // namespace performance
} // namespace streaming
// include/streaming/performance/parallelization.hpp
#pragma once

#include <vector>
#include <thread>
#include <future>
#include <atomic>
#include <queue>
#include <condition_variable>

namespace streaming {
namespace performance {

class ThreadPool {
public:
    ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();
    
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;
    
    void wait_all();
    size_t get_pending_tasks() const;
    size_t get_active_threads() const;

private:
    void worker_loop();
    
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};
    std::atomic<size_t> active_threads_{0};
};

template<typename F, typename... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type> {
    
    using return_type = typename std::result_of<F(Args...)>::type;
    
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> result = task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (stop_) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        tasks_.emplace([task](){ (*task)(); });
    }
    
    condition_.notify_one();
    return result;
}

class ParallelFor {
public:
    template<typename Func>
    static void execute(size_t start, size_t end, Func&& func, 
                       size_t min_chunk_size = 1000) {
        size_t total = end - start;
        if (total == 0) return;
        
        size_t num_chunks = std::min(total / min_chunk_size, 
                                   std::thread::hardware_concurrency());
        num_chunks = std::max(num_chunks, size_t(1));
        
        size_t chunk_size = (total + num_chunks - 1) / num_chunks;
        
        std::vector<std::future<void>> futures;
        futures.reserve(num_chunks);
        
        ThreadPool& pool = get_thread_pool();
        
        for (size_t i = 0; i < num_chunks; ++i) {
            size_t chunk_start = start + i * chunk_size;
            size_t chunk_end = std::min(chunk_start + chunk_size, end);
            
            if (chunk_start < chunk_end) {
                futures.emplace_back(
                    pool.enqueue([chunk_start, chunk_end, &func]() {
                        for (size_t j = chunk_start; j < chunk_end; ++j) {
                            func(j);
                        }
                    })
                );
            }
        }
        
        // Wait for all chunks to complete
        for (auto& future : futures) {
            future.wait();
        }
    }

private:
    static ThreadPool& get_thread_pool() {
        static ThreadPool pool(std::thread::hardware_concurrency());
        return pool;
    }
};

} // namespace performance
} // namespace streaming
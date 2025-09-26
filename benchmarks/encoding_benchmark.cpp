// benchmarks/encoding_benchmark.cpp
#include "streaming/performance/profiler.hpp"
#include "streaming/codec/h264_encoder.hpp"
#include "streaming/performance/optimizer.hpp"
#include <benchmark/benchmark.h>
#include <random>

using namespace streaming;

static void BM_H264_Encoding(benchmark::State& state) {
    codec::H264Encoder encoder;
    codec::VideoFrame frame;
    
    // Create test frame
    frame.width = 1920;
    frame.height = 1080;
    frame.data.resize(1920 * 1080 * 3);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& pixel : frame.data) {
        pixel = static_cast<uint8_t>(dis(gen));
    }
    
    performance::HighResProfiler::get_instance().start_session("H264_Benchmark");
    
    for (auto _ : state) {
        performance::HighResProfiler::get_instance().begin_sample("EncodeFrame");
        
        std::vector<uint8_t> output;
        encoder.encode_frame(frame, output);
        
        performance::HighResProfiler::get_instance().end_sample();
        
        benchmark::DoNotOptimize(output);
    }
    
    performance::HighResProfiler::get_instance().end_session();
    performance::HighResProfiler::get_instance().print_summary();
}

static void BM_Memory_Intensive_Operation(benchmark::State& state) {
    performance::CacheOptimizer cache_opt;
    const size_t size = state.range(0);
    
    std::vector<float> data(size);
    
    for (auto _ : state) {
        PROFILE_SCOPE("MemoryIntensive");
        
        // Test different access patterns
        for (size_t i = 0; i < size; ++i) {
            data[i] = std::sin(i * 0.1f) * std::cos(i * 0.05f);
        }
        
        benchmark::DoNotOptimize(data);
    }
    
    state.SetBytesProcessed(state.iterations() * size * sizeof(float));
}

// Register benchmarks
BENCHMARK(BM_H264_Encoding)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Memory_Intensive_Operation)
    ->Range(1 << 10, 1 << 20)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
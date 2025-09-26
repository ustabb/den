// examples/motion_estimation_test.cpp
#include "streaming/processing/motion_estimation.hpp"
#include <chrono>
#include <iostream>

using namespace streaming;

int main() {
    processing::MotionEstimator estimator;
    
    // Test frames oluştur (640x480)
    const int width = 640, height = 480;
    std::vector<uint8_t> frame1(width * height, 128);
    std::vector<uint8_t> frame2(width * height, 128);
    
    // Basit motion oluştur (sağa kaydır)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width - 5; ++x) {
            frame2[y * width + x + 5] = frame1[y * width + x];
        }
    }
    
    // Performance test
    auto start = std::chrono::high_resolution_clock::now();
    
    // Birkaç macroblock için motion estimation test et
    int blocks_processed = 0;
    for (int y = 16; y < height - 16; y += 16) {
        for (int x = 16; x < width - 16; x += 16) {
            auto mv = estimator.estimate_diamond_search(
                frame1.data(), frame2.data(), width, height, x, y
            );
            
            if (mv.valid) {
                std::cout << "Block (" << x << "," << y << "): MV=(" 
                          << mv.x << "," << mv.y << "), cost=" << mv.cost << "\n";
            }
            
            blocks_processed++;
            if (blocks_processed >= 10) break; // Kısa test
        }
        if (blocks_processed >= 10) break;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "\n✅ Motion estimation completed!\n";
    std::cout << "Time per block: " << duration.count() / blocks_processed << " μs\n";
    std::cout << "Blocks processed: " << blocks_processed << "\n";
    
    return 0;
}
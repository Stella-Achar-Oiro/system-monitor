#include "optimized_readers.h"
#include "header.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>

// Forward declarations for functions from implementation files
CPUStats readCPUStatsOptimized();
MemoryInfo readMemoryInfoOptimized();
vector<NetworkInterfaceStats> readNetworkInterfacesOptimized();
vector<ThermalSensor> discoverThermalSensorsOptimized();

// Traditional file reading for comparison
std::string read_traditional_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

void benchmark_file_reading() {
    const std::string test_file = "/proc/stat";
    const int iterations = 1000;
    
    std::cout << "=== File Reading Benchmark ===" << std::endl;
    
    // Traditional file reading
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        std::string content = read_traditional_file(test_file);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto traditional_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Traditional reading (" << iterations << " iterations): " << traditional_duration.count() << " μs" << std::endl;
    std::cout << "Average per read: " << traditional_duration.count() / float(iterations) << " μs" << std::endl;
    
    // Optimized file reading with caching
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        std::string content;
        g_proc_reader.read_file(test_file, content, 10); // 10ms cache
    }
    end = std::chrono::high_resolution_clock::now();
    auto optimized_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Optimized reading (" << iterations << " iterations): " << optimized_duration.count() << " μs" << std::endl;
    std::cout << "Average per read: " << optimized_duration.count() / float(iterations) << " μs" << std::endl;
    
    float speedup = float(traditional_duration.count()) / float(optimized_duration.count());
    std::cout << "Speedup: " << speedup << "x faster" << std::endl << std::endl;
}

void benchmark_data_structures() {
    std::cout << "=== Data Structure Benchmark ===" << std::endl;
    
    // Test BoundedDeque performance
    BoundedDeque<float> bounded(100);
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000; i++) {
        bounded.push_back(i * 0.1f);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "BoundedDeque (10000 insertions): " << duration.count() << " μs" << std::endl;
    
    // Test ThreadSafeContainer performance
    ThreadSafeContainer<float> container;
    
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; i++) {
        container.update(i * 0.1f);
        container.get(); // Just access, don't store
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "ThreadSafeContainer (1000 updates/gets): " << duration.count() << " μs" << std::endl << std::endl;
}

void benchmark_memory_usage() {
    std::cout << "=== Memory Usage Test ===" << std::endl;
    
    // Test memory usage with large data sets
    BoundedDeque<float> history(1000);
    for (int i = 0; i < 5000; i++) {
        history.push_back(i * 0.1f);
    }
    
    auto copy = history.get_copy();
    std::cout << "BoundedDeque size after 5000 insertions (max 1000): " << copy.size() << std::endl;
    
    // Test cache growth
    for (int i = 0; i < 100; i++) {
        std::string content;
        std::string path = "/proc/stat" + std::to_string(i); // Will fail, but tests cache behavior
        g_proc_reader.read_file(path, content);
    }
    
    std::cout << "Cache test completed (100 different paths)" << std::endl << std::endl;
}

int main() {
    std::cout << "=== Optimized System Monitor Performance Benchmark ===" << std::endl;
    std::cout << "Testing core optimization components..." << std::endl << std::endl;
    
    benchmark_file_reading();
    benchmark_data_structures();
    benchmark_memory_usage();
    
    // Print performance statistics
    g_perf_monitor.print_stats();
    
    return 0;
}
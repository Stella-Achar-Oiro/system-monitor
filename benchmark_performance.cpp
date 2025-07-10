#include "optimized_header.h"
#include <chrono>
#include <iostream>

void benchmark_cpu_reading() {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; i++) {
        CPUStats stats = readCPUStatsOptimized();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "CPU Reading (1000 iterations): " << duration.count() << " μs" << std::endl;
    std::cout << "Average per read: " << duration.count() / 1000.0 << " μs" << std::endl;
}

void benchmark_memory_reading() {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; i++) {
        MemoryInfo info = readMemoryInfoOptimized();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Memory Reading (1000 iterations): " << duration.count() << " μs" << std::endl;
    std::cout << "Average per read: " << duration.count() / 1000.0 << " μs" << std::endl;
}

void benchmark_network_reading() {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; i++) {
        auto interfaces = readNetworkInterfacesOptimized();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Network Reading (1000 iterations): " << duration.count() << " μs" << std::endl;
    std::cout << "Average per read: " << duration.count() / 1000.0 << " μs" << std::endl;
}

void benchmark_thermal_reading() {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; i++) {
        auto sensors = discoverThermalSensorsOptimized();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Thermal Reading (1000 iterations): " << duration.count() << " μs" << std::endl;
    std::cout << "Average per read: " << duration.count() / 1000.0 << " μs" << std::endl;
}

void benchmark_process_reading() {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10; i++) { // Fewer iterations for process reading as it's more expensive
        auto processes = readProcessListOptimized();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Process Reading (10 iterations): " << duration.count() << " μs" << std::endl;
    std::cout << "Average per read: " << duration.count() / 10.0 << " μs" << std::endl;
}

int main() {
    std::cout << "=== System Monitor Performance Benchmark ===" << std::endl;
    std::cout << "Testing optimized implementations..." << std::endl << std::endl;
    
    benchmark_cpu_reading();
    std::cout << std::endl;
    
    benchmark_memory_reading();
    std::cout << std::endl;
    
    benchmark_network_reading();
    std::cout << std::endl;
    
    benchmark_thermal_reading();
    std::cout << std::endl;
    
    benchmark_process_reading();
    std::cout << std::endl;
    
    // Test performance monitor
    g_perf_monitor.print_stats();
    
    return 0;
}
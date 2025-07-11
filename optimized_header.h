#ifndef OPTIMIZED_HEADER_H
#define OPTIMIZED_HEADER_H

#include "header.h"
#include "optimized_readers.h"
#include <thread>
#include <atomic>
#include <condition_variable>
#include <future>
#include <unistd.h>

// Function declarations for cross-distribution compatibility
void detectSystemCapabilities();

// Thread-safe monitoring structures
struct OptimizedCPUMonitor {
    ThreadSafeContainer<CPUStats> current_stats;
    ThreadSafeContainer<CPUStats> previous_stats;
    ThreadSafeContainer<float> current_cpu_percent;
    BoundedDeque<float> cpu_history;
    
    std::atomic<bool> is_paused{false};
    std::atomic<float> update_rate{60.0f};
    std::atomic<float> y_scale{100.0f};
    std::atomic<bool> is_first_read{true};
    
    std::chrono::steady_clock::time_point last_update_time;
    std::mutex update_mutex;
};

struct OptimizedThermalMonitor {
    ThreadSafeContainer<vector<ThermalSensor>> sensors;
    ThreadSafeContainer<float> current_max_temp;
    BoundedDeque<float> temp_history;
    
    std::atomic<bool> is_paused{false};
    std::atomic<float> update_rate{60.0f};
    std::atomic<float> y_scale{100.0f};
    
    std::chrono::steady_clock::time_point last_update_time;
    std::mutex update_mutex;
    std::string preferred_source;
};

struct OptimizedMemoryProcessMonitor {
    ThreadSafeContainer<MemoryInfo> memory;
    ThreadSafeContainer<DiskInfo> disk;
    ThreadSafeContainer<vector<ProcessInfo>> processes;
    ThreadSafeContainer<vector<ProcessInfo>> filtered_processes;
    
    ThreadSafeContainer<string> search_filter;
    std::atomic<bool> sort_by_cpu{true};
    std::atomic<bool> sort_by_memory{false};
    std::atomic<bool> sort_ascending{false};
    
    std::atomic<long long> total_system_memory{0};
    ThreadSafeContainer<map<int, ProcessInfo>> previous_processes;
    
    std::chrono::steady_clock::time_point last_update_time;
    std::mutex update_mutex;
};

struct OptimizedNetworkMonitor {
    ThreadSafeContainer<vector<NetworkInterfaceStats>> interfaces;
    ThreadSafeContainer<map<string, NetworkInterfaceStats>> previous_stats;
    BoundedDeque<float> rx_speed_history;
    BoundedDeque<float> tx_speed_history;
    
    std::atomic<float> max_rx_speed{0.0f};
    std::atomic<float> max_tx_speed{0.0f};
    std::atomic<bool> is_paused{false};
    std::atomic<float> update_rate{2.0f};
    
    ThreadSafeContainer<string> selected_interface;
    
    std::chrono::steady_clock::time_point last_update_time;
    std::mutex update_mutex;
};

// Data collection thread manager
class DataCollectionManager {
private:
    std::thread cpu_thread;
    std::thread thermal_thread;
    std::thread memory_thread;
    std::thread network_thread;
    
    std::atomic<bool> should_stop{false};
    std::condition_variable stop_cv;
    std::mutex stop_mutex;
    
    // Monitoring instances
    OptimizedCPUMonitor* cpu_monitor;
    OptimizedThermalMonitor* thermal_monitor;
    OptimizedMemoryProcessMonitor* memory_monitor;
    OptimizedNetworkMonitor* network_monitor;
    
    void cpu_collection_loop();
    void thermal_collection_loop();
    void memory_collection_loop();
    void network_collection_loop();
    
public:
    DataCollectionManager(OptimizedCPUMonitor* cpu, 
                         OptimizedThermalMonitor* thermal,
                         OptimizedMemoryProcessMonitor* memory,
                         OptimizedNetworkMonitor* network);
    ~DataCollectionManager();
    
    void start();
    void stop();
    bool is_running() const { return !should_stop.load(); }
};

// Optimized function declarations
CPUStats readCPUStatsOptimized();
float calculateCPUPercentOptimized(const CPUStats& current, const CPUStats& previous);
void updateCPUMonitorOptimized(OptimizedCPUMonitor& monitor);

vector<ThermalSensor> discoverThermalSensorsOptimized();
void updateThermalMonitorOptimized(OptimizedThermalMonitor& monitor);

MemoryInfo readMemoryInfoOptimized();
DiskInfo readDiskInfoOptimized(const string& mountpoint);
vector<ProcessInfo> readProcessListOptimized();
void updateMemoryProcessMonitorOptimized(OptimizedMemoryProcessMonitor& monitor);

vector<NetworkInterfaceStats> readNetworkInterfacesOptimized();
map<string, string> getInterfaceIPAddressesOptimized();
map<string, string> getInterfaceStatesOptimized();
void updateNetworkMonitorOptimized(OptimizedNetworkMonitor& monitor);

// Rendering functions for optimized structures
void renderOptimizedCPUGraph(OptimizedCPUMonitor& monitor);
void renderOptimizedThermalGraph(OptimizedThermalMonitor& monitor);
void renderOptimizedMemoryProcessInterface(OptimizedMemoryProcessMonitor& monitor);
void renderOptimizedNetworkInterface(OptimizedNetworkMonitor& monitor);

// Error handling and recovery
class ErrorHandler {
private:
    std::unordered_map<std::string, int> error_counts;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_errors;
    std::mutex error_mutex;
    
public:
    enum class ErrorLevel {
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };
    
    void log_error(const std::string& component, const std::string& message, ErrorLevel level = ErrorLevel::ERROR);
    bool should_retry(const std::string& component, int max_retries = 3, std::chrono::seconds cooldown = std::chrono::seconds(5));
    void reset_error_count(const std::string& component);
    void print_error_summary() const;
};

extern ErrorHandler g_error_handler;

// Memory pool for frequent allocations
template<typename T>
class ObjectPool {
private:
    std::vector<std::unique_ptr<T>> pool;
    std::mutex pool_mutex;
    size_t max_size;
    
public:
    ObjectPool(size_t max_sz = 100) : max_size(max_sz) {}
    
    std::unique_ptr<T> acquire() {
        std::lock_guard<std::mutex> lock(pool_mutex);
        if (!pool.empty()) {
            auto obj = std::move(pool.back());
            pool.pop_back();
            return obj;
        }
        return std::make_unique<T>();
    }
    
    void release(std::unique_ptr<T> obj) {
        std::lock_guard<std::mutex> lock(pool_mutex);
        if (pool.size() < max_size) {
            pool.push_back(std::move(obj));
        }
    }
};

// Global optimized instances
extern OptimizedCPUMonitor g_optimized_cpu_monitor;
extern OptimizedThermalMonitor g_optimized_thermal_monitor;
extern OptimizedMemoryProcessMonitor g_optimized_memory_process_monitor;
extern OptimizedNetworkMonitor g_optimized_network_monitor;
extern DataCollectionManager g_data_collection_manager;

#endif // OPTIMIZED_HEADER_H
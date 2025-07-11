#ifndef OPTIMIZED_READERS_H
#define OPTIMIZED_READERS_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <memory>
#include <chrono>
#include <functional>
#include <deque>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>

// Fast file reader using memory mapping for frequent reads
class FastFileReader {
private:
    std::string filename;
    int fd;
    char* mapped_data;
    size_t file_size;
    struct stat file_stat;
    std::chrono::steady_clock::time_point last_check;
    bool is_mapped;
    bool is_malloced; // true if we used malloc instead of mmap
    
public:
    FastFileReader(const std::string& file) 
        : filename(file), fd(-1), mapped_data(nullptr), file_size(0), is_mapped(false), is_malloced(false) {}
    
    ~FastFileReader() {
        cleanup();
    }
    
    void cleanup() {
        if (mapped_data) {
            if (is_malloced) {
                free(mapped_data);
            } else if (mapped_data != MAP_FAILED) {
                munmap(mapped_data, file_size);
            }
            mapped_data = nullptr;
        }
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
        is_mapped = false;
        is_malloced = false;
    }
    
    bool open() {
        fd = ::open(filename.c_str(), O_RDONLY);
        if (fd < 0) return false;
        
        if (fstat(fd, &file_stat) < 0) {
            close(fd);
            fd = -1;
            return false;
        }
        
        file_size = file_stat.st_size;
        
        // Handle /proc files which report size 0 but have content
        if (file_size == 0) {
            // Use traditional read for /proc files
            char buffer[4096];
            ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
            if (bytes_read <= 0) {
                close(fd);
                fd = -1;
                return false;
            }
            
            // Allocate memory for the content
            file_size = bytes_read;
            mapped_data = static_cast<char*>(malloc(file_size + 1));
            if (!mapped_data) {
                close(fd);
                fd = -1;
                return false;
            }
            
            memcpy(mapped_data, buffer, bytes_read);
            mapped_data[bytes_read] = '\0'; // Null terminate
            is_malloced = true;
        } else {
            // Use mmap for regular files
            mapped_data = static_cast<char*>(mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0));
            if (mapped_data == MAP_FAILED) {
                close(fd);
                fd = -1;
                return false;
            }
        }
        
        is_mapped = true;
        last_check = std::chrono::steady_clock::now();
        return true;
    }
    
    bool read_content(std::string& content) {
        if (!is_mapped) {
            if (!open()) return false;
        }
        
        // Check if file has been modified (for /proc files that might change)
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_check).count() > 100) {
            struct stat current_stat;
            if (fstat(fd, &current_stat) == 0) {
                if (current_stat.st_mtime != file_stat.st_mtime || current_stat.st_size != file_stat.st_size) {
                    cleanup();
                    if (!open()) return false;
                }
            }
            last_check = now;
        }
        
        content.assign(mapped_data, file_size);
        return true;
    }
};

// Buffered reader for frequently accessed files
class BufferedProcReader {
private:
    std::unordered_map<std::string, std::unique_ptr<FastFileReader>> readers;
    std::unordered_map<std::string, std::string> content_cache;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> cache_times;
    std::mutex cache_mutex;
    
public:
    bool read_file(const std::string& filename, std::string& content, int cache_ms = 50) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        
        auto now = std::chrono::steady_clock::now();
        
        // Check cache first
        auto cache_it = cache_times.find(filename);
        if (cache_it != cache_times.end()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - cache_it->second).count();
            if (elapsed < cache_ms) {
                content = content_cache[filename];
                return true;
            }
        }
        
        // Get or create reader
        auto reader_it = readers.find(filename);
        if (reader_it == readers.end()) {
            readers[filename] = std::make_unique<FastFileReader>(filename);
        }
        
        // Read fresh content
        if (readers[filename]->read_content(content)) {
            content_cache[filename] = content;
            cache_times[filename] = now;
            return true;
        }
        
        return false;
    }
    
    void clear_cache() {
        std::lock_guard<std::mutex> lock(cache_mutex);
        content_cache.clear();
        cache_times.clear();
        readers.clear();
    }
};

// Thread-safe data structures for monitoring
template<typename T>
class ThreadSafeContainer {
private:
    T data;
    mutable std::mutex mutex;
    
public:
    void update(const T& new_data) {
        std::lock_guard<std::mutex> lock(mutex);
        data = new_data;
    }
    
    T get() const {
        std::lock_guard<std::mutex> lock(mutex);
        return data;
    }
    
    void modify(std::function<void(T&)> modifier) {
        std::lock_guard<std::mutex> lock(mutex);
        modifier(data);
    }
};

// Optimized deque with size limiting
template<typename T>
class BoundedDeque {
private:
    std::deque<T> data;
    size_t max_size;
    mutable std::mutex mutex;
    
public:
    BoundedDeque(size_t max_sz = 200) : max_size(max_sz) {}
    
    void push_back(const T& item) {
        std::lock_guard<std::mutex> lock(mutex);
        data.push_back(item);
        while (data.size() > max_size) {
            data.pop_front();
        }
    }
    
    std::vector<T> get_copy() const {
        std::lock_guard<std::mutex> lock(mutex);
        return std::vector<T>(data.begin(), data.end());
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex);
        return data.empty();
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        data.clear();
    }
    
    void set_max_size(size_t new_max) {
        std::lock_guard<std::mutex> lock(mutex);
        max_size = new_max;
        while (data.size() > max_size) {
            data.pop_front();
        }
    }
};

// Performance monitoring
class PerformanceMonitor {
private:
    struct TimingData {
        std::chrono::steady_clock::time_point start_time;
        std::chrono::duration<double> total_time{0};
        uint64_t call_count = 0;
    };
    
    std::unordered_map<std::string, TimingData> timings;
    mutable std::mutex timing_mutex;
    
public:
    class Timer {
    private:
        PerformanceMonitor* monitor;
        std::string name;
        std::chrono::steady_clock::time_point start;
        
    public:
        Timer(PerformanceMonitor* mon, const std::string& n) 
            : monitor(mon), name(n), start(std::chrono::steady_clock::now()) {}
        
        ~Timer() {
            auto end = std::chrono::steady_clock::now();
            auto duration = end - start;
            monitor->record_timing(name, duration);
        }
    };
    
    void record_timing(const std::string& name, std::chrono::duration<double> duration) {
        std::lock_guard<std::mutex> lock(timing_mutex);
        auto& timing = timings[name];
        timing.total_time += duration;
        timing.call_count++;
    }
    
    Timer start_timer(const std::string& name) {
        return Timer(this, name);
    }
    
    void print_stats() const {
        std::lock_guard<std::mutex> lock(timing_mutex);
        std::cout << "\n=== Performance Statistics ===" << std::endl;
        for (const auto& [name, timing] : timings) {
            if (timing.call_count > 0) {
                auto avg_ms = (timing.total_time.count() / timing.call_count) * 1000.0;
                auto total_ms = timing.total_time.count() * 1000.0;
                std::cout << name << ": " << timing.call_count << " calls, "
                         << "avg: " << avg_ms << "ms, total: " << total_ms << "ms" << std::endl;
            }
        }
    }
    
    void reset() {
        std::lock_guard<std::mutex> lock(timing_mutex);
        timings.clear();
    }
};

// Global instances
extern BufferedProcReader g_proc_reader;
extern PerformanceMonitor g_perf_monitor;

#define PERF_TIMER(name) auto timer = g_perf_monitor.start_timer(name)

#endif // OPTIMIZED_READERS_H
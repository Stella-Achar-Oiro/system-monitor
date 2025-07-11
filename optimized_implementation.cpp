#include "optimized_header.h"
#include <sstream>
#include <fstream>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <net/if.h>
#include <dirent.h>

// Global instances
OptimizedCPUMonitor g_optimized_cpu_monitor;
OptimizedThermalMonitor g_optimized_thermal_monitor;
OptimizedMemoryProcessMonitor g_optimized_memory_process_monitor;
OptimizedNetworkMonitor g_optimized_network_monitor;
ErrorHandler g_error_handler;

// System capability detection for cross-distribution compatibility
struct SystemCapabilities {
    bool can_read_proc_stat = false;
    bool can_read_proc_meminfo = false;
    bool can_read_proc_net_dev = false;
    bool can_read_thermal_zones = false;
    bool can_read_hwmon = false;
    bool can_read_process_stats = false;
    bool has_proc_acpi_ibm = false;
    bool has_sysfs_thermal = false;
    bool has_sysfs_net = false;
    int page_size = 4096;
    std::string distribution = "unknown";
    std::string kernel_version = "unknown";
};

SystemCapabilities g_system_capabilities;

// Initialize system capabilities detection
void detectSystemCapabilities() {
    std::string test_content;
    
    // Test basic /proc access
    g_system_capabilities.can_read_proc_stat = g_proc_reader.read_file("/proc/stat", test_content, 10);
    g_system_capabilities.can_read_proc_meminfo = g_proc_reader.read_file("/proc/meminfo", test_content, 10);
    g_system_capabilities.can_read_proc_net_dev = g_proc_reader.read_file("/proc/net/dev", test_content, 10);
    
    // Test thermal sensor access
    g_system_capabilities.can_read_thermal_zones = g_proc_reader.read_file("/sys/class/thermal/thermal_zone0/temp", test_content, 10);
    g_system_capabilities.can_read_hwmon = g_proc_reader.read_file("/sys/class/hwmon/hwmon0/temp1_input", test_content, 10);
    g_system_capabilities.has_proc_acpi_ibm = g_proc_reader.read_file("/proc/acpi/ibm/thermal", test_content, 10);
    g_system_capabilities.has_sysfs_thermal = g_system_capabilities.can_read_thermal_zones;
    g_system_capabilities.has_sysfs_net = g_proc_reader.read_file("/sys/class/net/lo/statistics/rx_bytes", test_content, 10);
    
    // Test process access
    g_system_capabilities.can_read_process_stats = g_proc_reader.read_file("/proc/1/stat", test_content, 10);
    
    // Detect distribution
    if (g_proc_reader.read_file("/etc/os-release", test_content, 10)) {
        if (test_content.find("Ubuntu") != std::string::npos) {
            g_system_capabilities.distribution = "ubuntu";
        } else if (test_content.find("Debian") != std::string::npos) {
            g_system_capabilities.distribution = "debian";
        } else if (test_content.find("Fedora") != std::string::npos) {
            g_system_capabilities.distribution = "fedora";
        } else if (test_content.find("Red Hat") != std::string::npos || test_content.find("RHEL") != std::string::npos) {
            g_system_capabilities.distribution = "rhel";
        } else if (test_content.find("CentOS") != std::string::npos) {
            g_system_capabilities.distribution = "centos";
        } else if (test_content.find("Arch") != std::string::npos) {
            g_system_capabilities.distribution = "arch";
        } else if (test_content.find("openSUSE") != std::string::npos) {
            g_system_capabilities.distribution = "opensuse";
        } else if (test_content.find("Alpine") != std::string::npos) {
            g_system_capabilities.distribution = "alpine";
        }
    }
    
    // Detect page size
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size > 0) {
        g_system_capabilities.page_size = page_size;
    }
    
    // Get kernel version
    if (g_proc_reader.read_file("/proc/version", test_content, 10)) {
        // Extract version number (simplified)
        size_t version_pos = test_content.find("version ");
        if (version_pos != std::string::npos) {
            size_t space_pos = test_content.find(" ", version_pos + 8);
            if (space_pos != std::string::npos) {
                g_system_capabilities.kernel_version = test_content.substr(version_pos + 8, space_pos - version_pos - 8);
            }
        }
    }
    
    // Log capability detection results
    g_error_handler.log_error("System", "Detected distribution: " + g_system_capabilities.distribution, ErrorHandler::ErrorLevel::INFO);
    g_error_handler.log_error("System", "Page size: " + std::to_string(g_system_capabilities.page_size), ErrorHandler::ErrorLevel::INFO);
    
    if (!g_system_capabilities.can_read_proc_stat) {
        g_error_handler.log_error("System", "Cannot read /proc/stat - CPU monitoring may be limited", ErrorHandler::ErrorLevel::WARNING);
    }
    if (!g_system_capabilities.can_read_thermal_zones && !g_system_capabilities.has_proc_acpi_ibm) {
        g_error_handler.log_error("System", "Limited thermal sensor access detected", ErrorHandler::ErrorLevel::WARNING);
    }
}

DataCollectionManager g_data_collection_manager(
    &g_optimized_cpu_monitor,
    &g_optimized_thermal_monitor,
    &g_optimized_memory_process_monitor,
    &g_optimized_network_monitor
);

// Error handling implementation
void ErrorHandler::log_error(const std::string& component, const std::string& message, ErrorLevel level) {
    std::lock_guard<std::mutex> lock(error_mutex);
    error_counts[component]++;
    last_errors[component] = std::chrono::steady_clock::now();
    
    const char* level_str[] = {"INFO", "WARNING", "ERROR", "CRITICAL"};
    std::cerr << "[" << level_str[static_cast<int>(level)] << "] " 
              << component << ": " << message << std::endl;
}

bool ErrorHandler::should_retry(const std::string& component, int max_retries, std::chrono::seconds cooldown) {
    std::lock_guard<std::mutex> lock(error_mutex);
    
    auto it = error_counts.find(component);
    if (it == error_counts.end() || it->second < max_retries) {
        return true;
    }
    
    auto last_error_it = last_errors.find(component);
    if (last_error_it != last_errors.end()) {
        auto elapsed = std::chrono::steady_clock::now() - last_error_it->second;
        if (elapsed > cooldown) {
            error_counts[component] = 0;
            return true;
        }
    }
    
    return false;
}

void ErrorHandler::reset_error_count(const std::string& component) {
    std::lock_guard<std::mutex> lock(error_mutex);
    error_counts[component] = 0;
}

void ErrorHandler::print_error_summary() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(error_mutex));
    
    std::cout << "\n=== Error Summary ===" << std::endl;
    for (const auto& [component, count] : error_counts) {
        std::cout << component << ": " << count << " errors" << std::endl;
    }
    
    if (error_counts.empty()) {
        std::cout << "No errors recorded." << std::endl;
    }
    std::cout << std::endl;
}

// Optimized CPU monitoring
CPUStats readCPUStatsOptimized() {
    PERF_TIMER("readCPUStatsOptimized");
    
    CPUStats stats = {};
    std::string content;
    
    if (!g_proc_reader.read_file("/proc/stat", content)) {
        g_error_handler.log_error("CPU", "Failed to read /proc/stat");
        return stats;
    }
    
    std::istringstream iss(content);
    std::string line;
    if (std::getline(iss, line) && line.substr(0, 3) == "cpu") {
        std::istringstream cpu_stream(line);
        std::string cpu_label;
        cpu_stream >> cpu_label >> stats.user >> stats.nice >> stats.system 
                   >> stats.idle >> stats.iowait >> stats.irq >> stats.softirq 
                   >> stats.steal >> stats.guest >> stats.guestNice;
    }
    
    return stats;
}

float calculateCPUPercentOptimized(const CPUStats& current, const CPUStats& previous) {
    long long total_diff = current.getTotal() - previous.getTotal();
    long long idle_diff = current.getIdle() - previous.getIdle();
    
    if (total_diff == 0) return 0.0f;
    
    return ((float)(total_diff - idle_diff) / total_diff) * 100.0f;
}

void updateCPUMonitorOptimized(OptimizedCPUMonitor& monitor) {
    if (monitor.is_paused.load()) return;
    
    std::lock_guard<std::mutex> lock(monitor.update_mutex);
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - monitor.last_update_time).count();
    
    float update_interval = 1.0f / monitor.update_rate.load();
    if (elapsed < update_interval) return;
    
    CPUStats current = readCPUStatsOptimized();
    if (!monitor.is_first_read.load()) {
        CPUStats previous = monitor.previous_stats.get();
        float cpu_percent = calculateCPUPercentOptimized(current, previous);
        monitor.current_cpu_percent.update(cpu_percent);
        monitor.cpu_history.push_back(cpu_percent);
    } else {
        monitor.is_first_read.store(false);
    }
    
    monitor.previous_stats.update(current);
    monitor.last_update_time = now;
}

// Cross-distribution thermal monitoring with comprehensive fallback support
vector<ThermalSensor> discoverThermalSensorsOptimized() {
    PERF_TIMER("discoverThermalSensorsOptimized");
    
    vector<ThermalSensor> sensors;
    std::string content;
    
    // 1. Try ThinkPad ACPI thermal (ThinkPad-specific)
    if (g_proc_reader.read_file("/proc/acpi/ibm/thermal", content, 100)) {
        std::istringstream iss(content);
        std::string line;
        if (std::getline(iss, line) && line.find("temperatures:") != std::string::npos) {
            std::istringstream temp_stream(line);
            std::string word;
            temp_stream >> word; // "temperatures:"
            
            int temp_int;
            int sensor_num = 0;
            while (temp_stream >> temp_int && sensor_num < 8) {
                if (temp_int > 0 && temp_int < 200) { // Reasonable temperature range
                    ThermalSensor sensor;
                    sensor.name = "ThinkPad CPU" + (sensor_num > 0 ? " " + std::to_string(sensor_num) : "");
                    sensor.temperature = temp_int;
                    sensor.isValid = true;
                    sensor.source = "/proc/acpi/ibm/thermal";
                    sensors.push_back(sensor);
                }
                sensor_num++;
            }
        }
    }
    
    // 2. Try standard thermal zones (works on most modern Linux distributions)
    for (int i = 0; i < 50; i++) { // Increased to support more sensors
        std::string temp_path = "/sys/class/thermal/thermal_zone" + std::to_string(i) + "/temp";
        std::string type_path = "/sys/class/thermal/thermal_zone" + std::to_string(i) + "/type";
        
        if (g_proc_reader.read_file(temp_path, content, 100)) {
            try {
                int temp_milli = std::stoi(content);
                if (temp_milli > 0 && temp_milli < 200000) { // 0-200°C in millidegrees
                    ThermalSensor sensor;
                    sensor.temperature = temp_milli / 1000.0f;
                    sensor.isValid = true;
                    sensor.source = temp_path;
                    
                    // Try to get a descriptive name from type file
                    std::string type_content;
                    if (g_proc_reader.read_file(type_path, type_content, 100)) {
                        // Remove newlines and use as name
                        type_content.erase(std::remove(type_content.begin(), type_content.end(), '\n'), type_content.end());
                        sensor.name = type_content.empty() ? ("Thermal Zone " + std::to_string(i)) : type_content;
                    } else {
                        sensor.name = "Thermal Zone " + std::to_string(i);
                    }
                    
                    sensors.push_back(sensor);
                }
            } catch (...) {
                // Skip invalid temperature readings
                continue;
            }
        }
    }
    
    // 3. Try hwmon sensors (widely supported across distributions)
    for (int i = 0; i < 20; i++) {
        std::string hwmon_base = "/sys/class/hwmon/hwmon" + std::to_string(i);
        
        // Check multiple temperature inputs per hwmon device
        for (int j = 1; j <= 16; j++) {
            std::string temp_input = hwmon_base + "/temp" + std::to_string(j) + "_input";
            std::string temp_label = hwmon_base + "/temp" + std::to_string(j) + "_label";
            std::string name_file = hwmon_base + "/name";
            
            if (g_proc_reader.read_file(temp_input, content, 100)) {
                try {
                    int temp_milli = std::stoi(content);
                    if (temp_milli > 0 && temp_milli < 200000) { // 0-200°C in millidegrees
                        ThermalSensor sensor;
                        sensor.temperature = temp_milli / 1000.0f;
                        sensor.isValid = true;
                        sensor.source = temp_input;
                        
                        // Get sensor name from label or hwmon name
                        std::string label_content, name_content;
                        bool has_label = g_proc_reader.read_file(temp_label, label_content, 100);
                        bool has_name = g_proc_reader.read_file(name_file, name_content, 100);
                        
                        if (has_label && !label_content.empty()) {
                            label_content.erase(std::remove(label_content.begin(), label_content.end(), '\n'), label_content.end());
                            sensor.name = label_content;
                        } else if (has_name && !name_content.empty()) {
                            name_content.erase(std::remove(name_content.begin(), name_content.end(), '\n'), name_content.end());
                            sensor.name = name_content + " Temp" + std::to_string(j);
                        } else {
                            sensor.name = "hwmon" + std::to_string(i) + " Temp" + std::to_string(j);
                        }
                        
                        sensors.push_back(sensor);
                    }
                } catch (...) {
                    // Skip invalid temperature readings
                    continue;
                }
            }
        }
    }
    
    // 4. Try ACPI thermal zones (older systems and some distributions)
    for (int i = 0; i < 10; i++) {
        std::string acpi_path = "/proc/acpi/thermal_zone/THRM" + std::to_string(i) + "/temperature";
        if (g_proc_reader.read_file(acpi_path, content, 100)) {
            // Format: "temperature:             67 C"
            std::istringstream iss(content);
            std::string word;
            int temp;
            if (iss >> word >> temp && temp > 0 && temp < 200) {
                ThermalSensor sensor;
                sensor.name = "ACPI Thermal " + std::to_string(i);
                sensor.temperature = temp;
                sensor.isValid = true;
                sensor.source = acpi_path;
                sensors.push_back(sensor);
            }
        }
    }
    
    // 5. Try alternative ACPI thermal zone paths (distribution variations)
    std::vector<std::string> alt_acpi_paths = {
        "/proc/acpi/thermal_zone/TZ00/temperature",
        "/proc/acpi/thermal_zone/TZ01/temperature", 
        "/proc/acpi/thermal_zone/CPU0/temperature",
        "/proc/acpi/thermal_zone/CPU1/temperature",
        "/proc/acpi/thermal_zone/CPUZ/temperature"
    };
    
    for (const auto& path : alt_acpi_paths) {
        if (g_proc_reader.read_file(path, content, 100)) {
            std::istringstream iss(content);
            std::string word;
            int temp;
            if (iss >> word >> temp && temp > 0 && temp < 200) {
                ThermalSensor sensor;
                // Extract zone name from path
                size_t start = path.find_last_of('/');
                size_t end = path.find("/temperature");
                if (start != std::string::npos && end != std::string::npos) {
                    start = path.find_last_of('/', end - 1);
                    if (start != std::string::npos) {
                        sensor.name = path.substr(start + 1, end - start - 1);
                    } else {
                        sensor.name = "ACPI Zone";
                    }
                } else {
                    sensor.name = "ACPI Zone";
                }
                sensor.temperature = temp;
                sensor.isValid = true;
                sensor.source = path;
                sensors.push_back(sensor);
            }
        }
    }
    
    // 6. Try CPU-specific temperature sources (Intel/AMD specific paths)
    std::vector<std::string> cpu_temp_paths = {
        "/sys/devices/platform/coretemp.0/hwmon/hwmon*/temp*_input",
        "/sys/devices/platform/k10temp.0/hwmon/hwmon*/temp*_input",
        "/sys/devices/platform/w83627ehf.*/hwmon/hwmon*/temp*_input"
    };
    
    // Note: This would require glob expansion which we'll handle gracefully
    for (int i = 0; i < 10; i++) {
        std::string coretemp_path = "/sys/devices/platform/coretemp." + std::to_string(i);
        for (int j = 0; j < 5; j++) {
            for (int k = 1; k <= 8; k++) {
                std::string temp_path = coretemp_path + "/hwmon/hwmon" + std::to_string(j) + "/temp" + std::to_string(k) + "_input";
                std::string label_path = coretemp_path + "/hwmon/hwmon" + std::to_string(j) + "/temp" + std::to_string(k) + "_label";
                
                if (g_proc_reader.read_file(temp_path, content, 100)) {
                    try {
                        int temp_milli = std::stoi(content);
                        if (temp_milli > 0 && temp_milli < 200000) {
                            ThermalSensor sensor;
                            sensor.temperature = temp_milli / 1000.0f;
                            sensor.isValid = true;
                            sensor.source = temp_path;
                            
                            std::string label_content;
                            if (g_proc_reader.read_file(label_path, label_content, 100)) {
                                label_content.erase(std::remove(label_content.begin(), label_content.end(), '\n'), label_content.end());
                                sensor.name = label_content.empty() ? ("CPU Core " + std::to_string(k)) : label_content;
                            } else {
                                sensor.name = "CPU Core " + std::to_string(k);
                            }
                            
                            sensors.push_back(sensor);
                        }
                    } catch (...) {
                        continue;
                    }
                }
            }
        }
    }
    
    // 7. Gracefully handle permission issues - try alternative methods
    if (sensors.empty()) {
        g_error_handler.log_error("Thermal", "No thermal sensors found - checking permissions", ErrorHandler::ErrorLevel::WARNING);
        
        // Try sensors command if available (distribution fallback)
        // This would typically require external command execution
        // For now, we log the issue for user awareness
        g_error_handler.log_error("Thermal", "Consider running with appropriate permissions or installing lm-sensors", ErrorHandler::ErrorLevel::INFO);
    }
    
    return sensors;
}

void updateThermalMonitorOptimized(OptimizedThermalMonitor& monitor) {
    if (monitor.is_paused.load()) return;
    
    std::lock_guard<std::mutex> lock(monitor.update_mutex);
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - monitor.last_update_time).count();
    
    float update_interval = 1.0f / monitor.update_rate.load();
    if (elapsed < update_interval) return;
    
    vector<ThermalSensor> sensors = discoverThermalSensorsOptimized();
    monitor.sensors.update(sensors);
    
    float max_temp = 0.0f;
    for (const auto& sensor : sensors) {
        if (sensor.isValid && sensor.temperature > max_temp) {
            max_temp = sensor.temperature;
        }
    }
    
    monitor.current_max_temp.update(max_temp);
    monitor.temp_history.push_back(max_temp);
    monitor.last_update_time = now;
}

// Optimized memory monitoring
MemoryInfo readMemoryInfoOptimized() {
    PERF_TIMER("readMemoryInfoOptimized");
    
    MemoryInfo info = {};
    std::string content;
    
    if (!g_proc_reader.read_file("/proc/meminfo", content)) {
        g_error_handler.log_error("Memory", "Failed to read /proc/meminfo");
        return info;
    }
    
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        std::istringstream line_stream(line);
        std::string key;
        long long value;
        std::string unit;
        
        if (line_stream >> key >> value >> unit) {
            if (key == "MemTotal:") info.memTotal = value;
            else if (key == "MemFree:") info.memFree = value;
            else if (key == "MemAvailable:") info.memAvailable = value;
            else if (key == "Buffers:") info.buffers = value;
            else if (key == "Cached:") info.cached = value;
            else if (key == "SwapTotal:") info.swapTotal = value;
            else if (key == "SwapFree:") info.swapFree = value;
        }
    }
    
    info.swapUsed = info.swapTotal - info.swapFree;
    // Calculate used memory same as 'free' command: total - available
    // This matches the 'used' column in free command output
    info.memUsed = info.memTotal - info.memAvailable;
    info.memUsedPercent = info.memTotal > 0 ? (float)info.memUsed / info.memTotal * 100.0f : 0.0f;
    info.swapUsedPercent = info.swapTotal > 0 ? (float)info.swapUsed / info.swapTotal * 100.0f : 0.0f;
    
    return info;
}

// Cross-distribution process parsing with dynamic field detection
vector<ProcessInfo> readProcessListOptimized() {
    PERF_TIMER("readProcessListOptimized");
    
    vector<ProcessInfo> processes;
    
    // Check if we have permission to read process information
    if (!g_system_capabilities.can_read_process_stats) {
        g_error_handler.log_error("Process", "No permission to read process stats", ErrorHandler::ErrorLevel::WARNING);
        g_error_handler.log_error("Process", "Consider running with appropriate permissions", ErrorHandler::ErrorLevel::INFO);
        return processes;
    }
    
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        g_error_handler.log_error("Process", "Failed to open /proc directory");
        return processes;
    }
    
    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        // Check if directory name is numeric (PID)
        if (!isdigit(entry->d_name[0])) continue;
        
        int pid = atoi(entry->d_name);
        if (pid <= 0) continue;
        
        ProcessInfo process;
        process.pid = pid;
        
        // Read /proc/[pid]/stat with cross-distribution compatibility
        std::string stat_path = "/proc/" + std::string(entry->d_name) + "/stat";
        std::string content;
        
        if (!g_proc_reader.read_file(stat_path, content, 10)) {
            continue; // Process may have exited or permission denied
        }
        
        try {
            std::istringstream iss(content);
            std::string comm;
            std::vector<std::string> fields;
            std::string field;
            
            // Read all fields into vector for safer parsing
            while (iss >> field) {
                fields.push_back(field);
            }
            
            // Handle different kernel versions with varying field counts
            if (fields.size() < 20) {
                continue; // Not enough fields for basic process info
            }
            
            // Field 0: PID, Field 1: comm, Field 2: state
            process.pid = std::stoi(fields[0]);
            comm = fields[1];
            process.state = fields[2][0];
            
            // Fields 13-14: utime, stime (standard across kernel versions)
            if (fields.size() > 14) {
                process.utime = std::stoull(fields[13]);
                process.stime = std::stoull(fields[14]);
            }
            
            // Fields 22-23: vsize, rss (handle different kernel versions)
            if (fields.size() > 23) {
                process.vsize = std::stoull(fields[22]) / 1024; // Convert to KB
                // Use detected page size instead of hardcoded 4KB
                process.rss = std::stoull(fields[23]) * (g_system_capabilities.page_size / 1024);
            } else if (fields.size() > 22) {
                // Older kernels might have fewer fields
                process.vsize = std::stoull(fields[22]) / 1024;
                process.rss = 0; // Cannot determine RSS
            }
            
            // Extract process name from comm field (remove parentheses)
            if (comm.length() > 2 && comm[0] == '(' && comm.back() == ')') {
                process.name = comm.substr(1, comm.length() - 2);
            } else {
                process.name = comm;
            }
            
            // Distribution-specific adjustments
            if (g_system_capabilities.distribution == "alpine") {
                // Alpine Linux might use musl libc with different memory reporting
                // Adjust RSS calculation if needed
                if (process.rss == 0 && fields.size() > 23) {
                    process.rss = std::stoull(fields[23]) * 4; // Fallback to 4KB pages
                }
            }
            
            // Sanity checks adapted to system capabilities
            long max_memory_kb = 1024 * 1024 * 32; // 32GB default max
            if (g_system_capabilities.distribution == "alpine") {
                max_memory_kb = 1024 * 1024 * 8; // 8GB for Alpine (typically containers)
            }
            
            if (process.rss > max_memory_kb) {
                continue; // Skip unreasonable values
            }
            
            // Try to get additional process information if available
            std::string cmdline_path = "/proc/" + std::string(entry->d_name) + "/cmdline";
            std::string cmdline_content;
            if (g_proc_reader.read_file(cmdline_path, cmdline_content, 5)) {
                // Replace null terminators with spaces for better display
                for (char& c : cmdline_content) {
                    if (c == '\0') c = ' ';
                }
                // If cmdline is more descriptive than comm, use it (up to 32 chars)
                if (!cmdline_content.empty() && cmdline_content.length() > process.name.length()) {
                    process.name = cmdline_content.substr(0, 32);
                }
            }
            
            processes.push_back(process);
            
        } catch (const std::exception& e) {
            // Skip processes we can't parse
            continue;
        }
    }
    
    closedir(proc_dir);
    
    // Fallback: If we got very few processes, there might be permission issues
    if (processes.size() < 10) {
        g_error_handler.log_error("Process", "Found fewer than 10 processes - possible permission restrictions", ErrorHandler::ErrorLevel::WARNING);
        
        // Try to read at least basic system processes
        std::vector<int> essential_pids = {1, 2}; // init and kthreadd
        for (int pid : essential_pids) {
            std::string stat_path = "/proc/" + std::to_string(pid) + "/stat";
            std::string content;
            if (g_proc_reader.read_file(stat_path, content, 10)) {
                // Add basic process info even if we can't get full details
                ProcessInfo process;
                process.pid = pid;
                process.name = (pid == 1) ? "init" : "kthreadd";
                process.state = 'S';
                process.rss = 0;
                process.vsize = 0;
                process.utime = 0;
                process.stime = 0;
                
                // Check if this process is already in our list
                bool found = false;
                for (const auto& existing : processes) {
                    if (existing.pid == pid) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    processes.push_back(process);
                }
            }
        }
    }
    
    return processes;
}

void updateMemoryProcessMonitorOptimized(OptimizedMemoryProcessMonitor& monitor) {
    std::lock_guard<std::mutex> lock(monitor.update_mutex);
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - monitor.last_update_time).count();
    
    if (elapsed < 0.5f) return; // Update every 500ms
    
    MemoryInfo memory = readMemoryInfoOptimized();
    monitor.memory.update(memory);
    
    // Update total system memory
    monitor.total_system_memory.store(memory.memTotal);
    
    vector<ProcessInfo> processes = readProcessListOptimized();
    
    // Calculate CPU percentages using previous data
    map<int, ProcessInfo> prev_processes = monitor.previous_processes.get();
    for (auto& process : processes) {
        auto it = prev_processes.find(process.pid);
        if (it != prev_processes.end()) {
            long long cpu_time_diff = (process.utime + process.stime) - (it->second.utime + it->second.stime);
            if (elapsed > 0) {
                process.cpuPercent = (cpu_time_diff / (elapsed * 100.0f)) * 100.0f; // Rough approximation
            }
        }
        
        // Calculate memory percentage
        if (memory.memTotal > 0) {
            process.memPercent = (float)process.rss / (memory.memTotal) * 100.0f;
        }
    }
    
    monitor.processes.update(processes);
    
    // Store for next calculation
    map<int, ProcessInfo> current_map;
    for (const auto& process : processes) {
        current_map[process.pid] = process;
    }
    monitor.previous_processes.update(current_map);
    
    monitor.last_update_time = now;
}

// Cross-distribution network interface parsing with comprehensive format support
vector<NetworkInterfaceStats> readNetworkInterfacesOptimized() {
    PERF_TIMER("readNetworkInterfacesOptimized");
    
    vector<NetworkInterfaceStats> interfaces;
    std::string content;
    
    // Primary method: /proc/net/dev (standard across all distributions)
    if (g_proc_reader.read_file("/proc/net/dev", content)) {
        std::istringstream iss(content);
        std::string line;
        int line_count = 0;
        
        while (std::getline(iss, line)) {
            line_count++;
            if (line_count <= 2) continue; // Skip header lines
            
            // Find the colon that separates interface name from stats
            size_t colon_pos = line.find(':');
            if (colon_pos == std::string::npos) continue;
            
            NetworkInterfaceStats interface;
            
            // Extract interface name and trim whitespace
            std::string iface_name = line.substr(0, colon_pos);
            size_t start = iface_name.find_first_not_of(" \t");
            size_t end = iface_name.find_last_not_of(" \t");
            if (start != std::string::npos && end != std::string::npos) {
                interface.name = iface_name.substr(start, end - start + 1);
            } else {
                continue; // Invalid interface name
            }
            
            // Parse statistics - handle varying field counts gracefully
            std::string stats = line.substr(colon_pos + 1);
            std::istringstream stats_stream(stats);
            
            std::vector<long long> values;
            long long value;
            while (stats_stream >> value && values.size() < 20) { // Allow up to 20 fields for future compatibility
                values.push_back(value);
            }
            
            // Support different /proc/net/dev formats across kernel versions
            if (values.size() >= 16) {
                // Standard format (16 fields)
                interface.rxBytes = values[0];
                interface.rxPackets = values[1];
                interface.rxErrs = values[2];
                interface.rxDrop = values[3];
                interface.rxFifo = values[4];
                interface.rxFrame = values[5];
                interface.rxCompressed = values[6];
                interface.rxMulticast = values[7];
                
                interface.txBytes = values[8];
                interface.txPackets = values[9];
                interface.txErrs = values[10];
                interface.txDrop = values[11];
                interface.txFifo = values[12];
                interface.txColls = values[13];
                interface.txCarrier = values[14];
                interface.txCompressed = values[15];
            } else if (values.size() >= 10) {
                // Minimal format (older kernels or limited interfaces)
                interface.rxBytes = values[0];
                interface.rxPackets = values[1];
                interface.rxErrs = values.size() > 2 ? values[2] : 0;
                interface.rxDrop = values.size() > 3 ? values[3] : 0;
                interface.rxFifo = 0;
                interface.rxFrame = 0;
                interface.rxCompressed = 0;
                interface.rxMulticast = 0;
                
                // TX stats start at different positions depending on available RX fields
                int tx_start = std::min(8, (int)values.size() - 2);
                interface.txBytes = values.size() > tx_start ? values[tx_start] : 0;
                interface.txPackets = values.size() > tx_start + 1 ? values[tx_start + 1] : 0;
                interface.txErrs = 0;
                interface.txDrop = 0;
                interface.txFifo = 0;
                interface.txColls = 0;
                interface.txCarrier = 0;
                interface.txCompressed = 0;
            } else {
                continue; // Not enough data
            }
            
            // Sanity check for reasonable values
            if (interface.rxBytes >= 0 && interface.txBytes >= 0) {
                interfaces.push_back(interface);
            }
        }
    } else {
        g_error_handler.log_error("Network", "Failed to read /proc/net/dev", ErrorHandler::ErrorLevel::WARNING);
    }
    
    // Fallback method: sysfs network interfaces (when /proc/net/dev is unavailable)
    if (interfaces.empty()) {
        g_error_handler.log_error("Network", "Trying sysfs fallback for network interfaces", ErrorHandler::ErrorLevel::INFO);
        
        // Try to read from /sys/class/net/*/statistics/*
        for (int i = 0; i < 100; i++) { // Check common interface patterns
            std::vector<std::string> common_names = {
                "eth" + std::to_string(i),
                "wlan" + std::to_string(i),
                "wlp" + std::to_string(i) + "s0",
                "enp" + std::to_string(i) + "s0",
                "ens" + std::to_string(i),
                "lo"
            };
            
            for (const auto& name : common_names) {
                std::string base_path = "/sys/class/net/" + name + "/statistics/";
                std::string rx_bytes_path = base_path + "rx_bytes";
                std::string tx_bytes_path = base_path + "tx_bytes";
                
                std::string rx_content, tx_content;
                if (g_proc_reader.read_file(rx_bytes_path, rx_content, 50) &&
                    g_proc_reader.read_file(tx_bytes_path, tx_content, 50)) {
                    
                    try {
                        NetworkInterfaceStats interface;
                        interface.name = name;
                        interface.rxBytes = std::stoll(rx_content);
                        interface.txBytes = std::stoll(tx_content);
                        
                        // Try to read additional statistics
                        std::string temp_content;
                        if (g_proc_reader.read_file(base_path + "rx_packets", temp_content, 50)) {
                            interface.rxPackets = std::stoll(temp_content);
                        }
                        if (g_proc_reader.read_file(base_path + "tx_packets", temp_content, 50)) {
                            interface.txPackets = std::stoll(temp_content);
                        }
                        if (g_proc_reader.read_file(base_path + "rx_errors", temp_content, 50)) {
                            interface.rxErrs = std::stoll(temp_content);
                        }
                        if (g_proc_reader.read_file(base_path + "tx_errors", temp_content, 50)) {
                            interface.txErrs = std::stoll(temp_content);
                        }
                        if (g_proc_reader.read_file(base_path + "rx_dropped", temp_content, 50)) {
                            interface.rxDrop = std::stoll(temp_content);
                        }
                        if (g_proc_reader.read_file(base_path + "tx_dropped", temp_content, 50)) {
                            interface.txDrop = std::stoll(temp_content);
                        }
                        
                        interfaces.push_back(interface);
                    } catch (...) {
                        continue; // Skip invalid entries
                    }
                }
            }
        }
    }
    
    // Additional fallback: Try to discover interfaces dynamically
    if (interfaces.empty()) {
        g_error_handler.log_error("Network", "Trying dynamic interface discovery", ErrorHandler::ErrorLevel::INFO);
        
        // This could be enhanced to read /sys/class/net/ directory listing
        // For now, we provide comprehensive static fallbacks
        std::vector<std::string> fallback_interfaces = {
            "lo", "eth0", "eth1", "wlan0", "wlan1", "wlp2s0", "wlp3s0", 
            "enp0s31f6", "ens33", "ens160", "br0", "docker0", "virbr0"
        };
        
        for (const auto& name : fallback_interfaces) {
            std::string base_path = "/sys/class/net/" + name + "/statistics/";
            std::string rx_bytes_path = base_path + "rx_bytes";
            std::string tx_bytes_path = base_path + "tx_bytes";
            
            std::string rx_content, tx_content;
            if (g_proc_reader.read_file(rx_bytes_path, rx_content, 50) &&
                g_proc_reader.read_file(tx_bytes_path, tx_content, 50)) {
                
                try {
                    NetworkInterfaceStats interface;
                    interface.name = name;
                    interface.rxBytes = std::stoll(rx_content);
                    interface.txBytes = std::stoll(tx_content);
                    
                    // Minimal stats for fallback
                    interface.rxPackets = 0;
                    interface.txPackets = 0;
                    interface.rxErrs = 0;
                    interface.txErrs = 0;
                    interface.rxDrop = 0;
                    interface.txDrop = 0;
                    interface.rxFifo = 0;
                    interface.txFifo = 0;
                    interface.rxFrame = 0;
                    interface.txColls = 0;
                    interface.rxCompressed = 0;
                    interface.txCompressed = 0;
                    interface.rxMulticast = 0;
                    interface.txCarrier = 0;
                    
                    interfaces.push_back(interface);
                } catch (...) {
                    continue;
                }
            }
        }
    }
    
    // Final fallback: Handle permission issues gracefully
    if (interfaces.empty()) {
        g_error_handler.log_error("Network", "No network interfaces found - possible permission issue", ErrorHandler::ErrorLevel::WARNING);
        g_error_handler.log_error("Network", "Consider running with appropriate permissions", ErrorHandler::ErrorLevel::INFO);
        
        // Create a minimal loopback interface for basic functionality
        NetworkInterfaceStats lo_interface;
        lo_interface.name = "lo (fallback)";
        lo_interface.rxBytes = 0;
        lo_interface.txBytes = 0;
        lo_interface.rxPackets = 0;
        lo_interface.txPackets = 0;
        // Initialize all other fields to 0
        interfaces.push_back(lo_interface);
    }
    
    return interfaces;
}

map<string, string> getInterfaceIPAddressesOptimized() {
    PERF_TIMER("getInterfaceIPAddressesOptimized");
    
    map<string, string> ip_addresses;
    
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];
    
    if (getifaddrs(&ifaddr) == -1) {
        g_error_handler.log_error("Network", "Failed to get interface addresses");
        return ip_addresses;
    }
    
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        
        int family = ifa->ifa_addr->sa_family;
        if (family == AF_INET) {
            int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                              host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
            if (s == 0) {
                ip_addresses[ifa->ifa_name] = host;
            }
        }
    }
    
    freeifaddrs(ifaddr);
    return ip_addresses;
}

map<string, string> getInterfaceStatesOptimized() {
    PERF_TIMER("getInterfaceStatesOptimized");
    
    map<string, string> states;
    
    struct ifaddrs *ifaddr, *ifa;
    
    if (getifaddrs(&ifaddr) == -1) {
        g_error_handler.log_error("Network", "Failed to get interface addresses");
        return states;
    }
    
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        
        if (ifa->ifa_flags & IFF_UP) {
            if (ifa->ifa_flags & IFF_RUNNING) {
                states[ifa->ifa_name] = "UP";
            } else {
                states[ifa->ifa_name] = "DOWN";
            }
        } else {
            states[ifa->ifa_name] = "DOWN";
        }
    }
    
    freeifaddrs(ifaddr);
    return states;
}

DiskInfo readDiskInfoOptimized(const string& mountpoint) {
    PERF_TIMER("readDiskInfoOptimized");
    
    DiskInfo info = {};
    struct statvfs vfs;
    
    if (statvfs(mountpoint.c_str(), &vfs) == 0) {
        info.total = (vfs.f_blocks * vfs.f_frsize) / 1024;
        info.available = (vfs.f_bavail * vfs.f_frsize) / 1024;
        info.used = info.total - info.available;
        info.usedPercent = info.total > 0 ? (float)info.used / info.total * 100.0f : 0.0f;
        info.mountpoint = mountpoint;
    } else {
        g_error_handler.log_error("Disk", "Failed to read disk info for " + mountpoint);
    }
    
    return info;
}

void updateNetworkMonitorOptimized(OptimizedNetworkMonitor& monitor) {
    if (monitor.is_paused.load()) return;
    
    std::lock_guard<std::mutex> lock(monitor.update_mutex);
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - monitor.last_update_time).count();
    
    float update_interval = 1.0f / monitor.update_rate.load();
    if (elapsed < update_interval) return;
    
    vector<NetworkInterfaceStats> interfaces = readNetworkInterfacesOptimized();
    map<string, NetworkInterfaceStats> prev_stats = monitor.previous_stats.get();
    
    // Calculate speeds
    for (auto& interface : interfaces) {
        auto it = prev_stats.find(interface.name);
        if (it != prev_stats.end() && elapsed > 0) {
            long long rx_delta = interface.rxBytes - it->second.rxBytes;
            long long tx_delta = interface.txBytes - it->second.txBytes;
            
            interface.rxSpeed = rx_delta / elapsed;
            interface.txSpeed = tx_delta / elapsed;
            
            // Update max speeds
            float current_max_rx = monitor.max_rx_speed.load();
            if (interface.rxSpeed > current_max_rx) {
                monitor.max_rx_speed.store(interface.rxSpeed);
            }
            
            float current_max_tx = monitor.max_tx_speed.load();
            if (interface.txSpeed > current_max_tx) {
                monitor.max_tx_speed.store(interface.txSpeed);
            }
        }
    }
    
    monitor.interfaces.update(interfaces);
    
    // Update previous stats
    map<string, NetworkInterfaceStats> new_prev_stats;
    for (const auto& interface : interfaces) {
        new_prev_stats[interface.name] = interface;
    }
    monitor.previous_stats.update(new_prev_stats);
    
    // Update speed history for selected interface or total
    string selected = monitor.selected_interface.get();
    float total_rx = 0, total_tx = 0;
    
    for (const auto& interface : interfaces) {
        if (interface.name != "lo") { // Skip loopback
            total_rx += interface.rxSpeed;
            total_tx += interface.txSpeed;
        }
    }
    
    monitor.rx_speed_history.push_back(total_rx);
    monitor.tx_speed_history.push_back(total_tx);
    
    monitor.last_update_time = now;
}

// Data collection manager implementation
DataCollectionManager::DataCollectionManager(OptimizedCPUMonitor* cpu, 
                                           OptimizedThermalMonitor* thermal,
                                           OptimizedMemoryProcessMonitor* memory,
                                           OptimizedNetworkMonitor* network)
    : cpu_monitor(cpu), thermal_monitor(thermal), memory_monitor(memory), network_monitor(network) {}

DataCollectionManager::~DataCollectionManager() {
    stop();
}

void DataCollectionManager::start() {
    should_stop.store(false);
    
    cpu_thread = std::thread(&DataCollectionManager::cpu_collection_loop, this);
    thermal_thread = std::thread(&DataCollectionManager::thermal_collection_loop, this);
    memory_thread = std::thread(&DataCollectionManager::memory_collection_loop, this);
    network_thread = std::thread(&DataCollectionManager::network_collection_loop, this);
}

void DataCollectionManager::stop() {
    should_stop.store(true);
    stop_cv.notify_all();
    
    if (cpu_thread.joinable()) cpu_thread.join();
    if (thermal_thread.joinable()) thermal_thread.join();
    if (memory_thread.joinable()) memory_thread.join();
    if (network_thread.joinable()) network_thread.join();
}

void DataCollectionManager::cpu_collection_loop() {
    while (!should_stop.load()) {
        try {
            updateCPUMonitorOptimized(*cpu_monitor);
        } catch (const std::exception& e) {
            g_error_handler.log_error("CPU Thread", e.what());
        }
        
        std::unique_lock<std::mutex> lock(stop_mutex);
        stop_cv.wait_for(lock, std::chrono::milliseconds(50));
    }
}

void DataCollectionManager::thermal_collection_loop() {
    while (!should_stop.load()) {
        try {
            updateThermalMonitorOptimized(*thermal_monitor);
        } catch (const std::exception& e) {
            g_error_handler.log_error("Thermal Thread", e.what());
        }
        
        std::unique_lock<std::mutex> lock(stop_mutex);
        stop_cv.wait_for(lock, std::chrono::milliseconds(100));
    }
}

void DataCollectionManager::memory_collection_loop() {
    while (!should_stop.load()) {
        try {
            updateMemoryProcessMonitorOptimized(*memory_monitor);
        } catch (const std::exception& e) {
            g_error_handler.log_error("Memory Thread", e.what());
        }
        
        std::unique_lock<std::mutex> lock(stop_mutex);
        stop_cv.wait_for(lock, std::chrono::milliseconds(500));
    }
}

void DataCollectionManager::network_collection_loop() {
    while (!should_stop.load()) {
        try {
            updateNetworkMonitorOptimized(*network_monitor);
        } catch (const std::exception& e) {
            g_error_handler.log_error("Network Thread", e.what());
        }
        
        std::unique_lock<std::mutex> lock(stop_mutex);
        stop_cv.wait_for(lock, std::chrono::milliseconds(250));
    }
}
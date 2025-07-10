// Additional robust parsing fixes for different system configurations
#include "optimized_header.h"
#include <sstream>
#include <algorithm>

// Enhanced thermal sensor discovery with fallbacks
vector<ThermalSensor> discoverThermalSensorsRobust() {
    vector<ThermalSensor> sensors;
    
    // 1. Try ThinkPad ACPI thermal (most reliable on ThinkPads)
    std::string content;
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
    
    // 2. Try standard thermal zones (works on most systems)
    for (int i = 0; i < 20; i++) {
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
    
    // 3. Try ACPI thermal zones (alternative method)
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
    
    return sensors;
}

// Enhanced process parsing with error handling
vector<ProcessInfo> readProcessListRobust() {
    vector<ProcessInfo> processes;
    
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
        
        // Read /proc/[pid]/stat with robust parsing
        std::string stat_path = "/proc/" + std::string(entry->d_name) + "/stat";
        std::string content;
        
        if (!g_proc_reader.read_file(stat_path, content, 10)) {
            continue; // Process may have exited
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
            
            if (fields.size() < 24) {
                continue; // Not enough fields, skip this process
            }
            
            // Field 0: PID, Field 1: comm, Field 2: state
            process.pid = std::stoi(fields[0]);
            comm = fields[1];
            process.state = fields[2][0];
            
            // Fields 13-14: utime, stime
            if (fields.size() > 14) {
                process.utime = std::stoull(fields[13]);
                process.stime = std::stoull(fields[14]);
            }
            
            // Fields 22-23: vsize, rss  
            if (fields.size() > 23) {
                process.vsize = std::stoull(fields[22]) / 1024; // Convert to KB
                process.rss = std::stoull(fields[23]) * 4;      // Convert pages to KB
            }
            
            // Extract process name from comm field (remove parentheses)
            if (comm.length() > 2 && comm[0] == '(' && comm.back() == ')') {
                process.name = comm.substr(1, comm.length() - 2);
            } else {
                process.name = comm;
            }
            
            // Sanity checks
            if (process.rss > 1024 * 1024 * 16) { // More than 16GB seems unreasonable
                continue;
            }
            
            processes.push_back(process);
            
        } catch (const std::exception& e) {
            // Skip processes we can't parse
            continue;
        }
    }
    
    closedir(proc_dir);
    return processes;
}

// Enhanced network interface parsing with error handling
vector<NetworkInterfaceStats> readNetworkInterfacesRobust() {
    vector<NetworkInterfaceStats> interfaces;
    std::string content;
    
    if (!g_proc_reader.read_file("/proc/net/dev", content)) {
        g_error_handler.log_error("Network", "Failed to read /proc/net/dev");
        return interfaces;
    }
    
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
        
        // Parse statistics
        std::string stats = line.substr(colon_pos + 1);
        std::istringstream stats_stream(stats);
        
        // Expect 16 numeric fields
        std::vector<long long> values;
        long long value;
        while (stats_stream >> value && values.size() < 16) {
            values.push_back(value);
        }
        
        if (values.size() >= 16) {
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
            
            // Sanity check for reasonable values
            if (interface.rxBytes >= 0 && interface.txBytes >= 0) {
                interfaces.push_back(interface);
            }
        }
    }
    
    return interfaces;
}
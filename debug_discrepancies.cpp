#include "header.h"
#include "optimized_header.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

// Comparison functions for debugging
void debug_process_data() {
    std::cout << "=== PROCESS DATA DEBUGGING ===" << std::endl;
    
    // Get system monitor data
    auto processes = readProcessListOptimized();
    std::cout << "System Monitor - Total processes: " << processes.size() << std::endl;
    
    // Get top command data
    std::cout << "\nRunning 'ps aux | wc -l' for comparison:" << std::endl;
    system("ps aux | wc -l");
    
    std::cout << "\nRunning 'ps -eo pid,comm,state,%cpu,%mem,rss --no-headers | wc -l' for comparison:" << std::endl;
    system("ps -eo pid,comm,state,%cpu,%mem,rss --no-headers | wc -l");
    
    // Show sample of our process data
    std::cout << "\nSystem Monitor - Sample processes (first 5):" << std::endl;
    std::cout << std::setw(8) << "PID" << std::setw(16) << "Name" << std::setw(8) << "State" 
              << std::setw(10) << "CPU%" << std::setw(10) << "Mem%" << std::setw(12) << "RSS(KB)" << std::endl;
    
    for (int i = 0; i < std::min(5, (int)processes.size()); i++) {
        const auto& p = processes[i];
        std::cout << std::setw(8) << p.pid << std::setw(16) << p.name.substr(0,15) 
                  << std::setw(8) << p.state << std::setw(10) << std::fixed << std::setprecision(1) << p.cpuPercent
                  << std::setw(10) << p.memPercent << std::setw(12) << p.rss << std::endl;
    }
    
    std::cout << "\nCompare with 'ps -eo pid,comm,state,%cpu,%mem,rss --no-headers | head -5':" << std::endl;
    system("ps -eo pid,comm,state,%cpu,%mem,rss --no-headers | head -5");
    
    std::cout << std::endl;
}

void debug_memory_data() {
    std::cout << "=== MEMORY DATA DEBUGGING ===" << std::endl;
    
    // Get system monitor data
    MemoryInfo memory = readMemoryInfoOptimized();
    
    std::cout << "System Monitor Memory Data:" << std::endl;
    std::cout << "  Total:     " << formatBytes(memory.memTotal * 1024) << " (" << memory.memTotal << " KB)" << std::endl;
    std::cout << "  Free:      " << formatBytes(memory.memFree * 1024) << " (" << memory.memFree << " KB)" << std::endl;
    std::cout << "  Available: " << formatBytes(memory.memAvailable * 1024) << " (" << memory.memAvailable << " KB)" << std::endl;
    std::cout << "  Used:      " << formatBytes(memory.memUsed * 1024) << " (" << memory.memUsed << " KB)" << std::endl;
    std::cout << "  Buffers:   " << formatBytes(memory.buffers * 1024) << " (" << memory.buffers << " KB)" << std::endl;
    std::cout << "  Cached:    " << formatBytes(memory.cached * 1024) << " (" << memory.cached << " KB)" << std::endl;
    std::cout << "  Used%:     " << std::fixed << std::setprecision(1) << memory.memUsedPercent << "%" << std::endl;
    
    if (memory.swapTotal > 0) {
        std::cout << "  Swap Total: " << formatBytes(memory.swapTotal * 1024) << " (" << memory.swapTotal << " KB)" << std::endl;
        std::cout << "  Swap Used:  " << formatBytes(memory.swapUsed * 1024) << " (" << memory.swapUsed << " KB)" << std::endl;
        std::cout << "  Swap%:      " << memory.swapUsedPercent << "%" << std::endl;
    }
    
    std::cout << "\nCompare with 'free -k':" << std::endl;
    system("free -k");
    
    std::cout << "\nCompare with 'free -h':" << std::endl;
    system("free -h");
    
    std::cout << "\nRaw /proc/meminfo (first 10 lines):" << std::endl;
    system("head -10 /proc/meminfo");
    
    std::cout << std::endl;
}

void debug_network_data() {
    std::cout << "=== NETWORK DATA DEBUGGING ===" << std::endl;
    
    // Get system monitor data
    auto interfaces = readNetworkInterfacesOptimized();
    
    std::cout << "System Monitor Network Data:" << std::endl;
    std::cout << std::setw(12) << "Interface" << std::setw(15) << "RX Bytes" << std::setw(15) << "TX Bytes" 
              << std::setw(12) << "RX Packets" << std::setw(12) << "TX Packets" << std::endl;
    
    for (const auto& iface : interfaces) {
        std::cout << std::setw(12) << iface.name 
                  << std::setw(15) << iface.rxBytes 
                  << std::setw(15) << iface.txBytes
                  << std::setw(12) << iface.rxPackets
                  << std::setw(12) << iface.txPackets << std::endl;
    }
    
    std::cout << "\nCompare with raw /proc/net/dev:" << std::endl;
    system("cat /proc/net/dev");
    
    std::cout << "\nInterface states check:" << std::endl;
    auto ip_addresses = getInterfaceIPAddressesOptimized();
    auto states = getInterfaceStatesOptimized();
    
    for (const auto& iface : interfaces) {
        std::cout << iface.name << ": ";
        auto ip_it = ip_addresses.find(iface.name);
        auto state_it = states.find(iface.name);
        
        if (state_it != states.end()) {
            std::cout << "State=" << state_it->second << " ";
        }
        if (ip_it != ip_addresses.end()) {
            std::cout << "IP=" << ip_it->second;
        }
        std::cout << std::endl;
    }
    
    std::cout << "\nCompare with 'ip addr show':" << std::endl;
    system("ip addr show | grep -E '^[0-9]+:|inet '");
    
    std::cout << std::endl;
}

void debug_thermal_data() {
    std::cout << "=== THERMAL DATA DEBUGGING ===" << std::endl;
    
    // Get system monitor data
    auto sensors = discoverThermalSensorsOptimized();
    
    std::cout << "System Monitor Thermal Data:" << std::endl;
    for (const auto& sensor : sensors) {
        if (sensor.isValid) {
            std::cout << "  " << sensor.name << ": " << std::fixed << std::setprecision(1) 
                      << sensor.temperature << "°C (source: " << sensor.source << ")" << std::endl;
        }
    }
    
    std::cout << "\nChecking thermal sources:" << std::endl;
    
    std::cout << "\n1. /proc/acpi/ibm/thermal (ThinkPad specific):" << std::endl;
    system("cat /proc/acpi/ibm/thermal 2>/dev/null || echo 'Not available'");
    
    std::cout << "\n2. /sys/class/thermal/thermal_zone*/temp:" << std::endl;
    system("find /sys/class/thermal -name 'thermal_zone*/temp' -exec sh -c 'echo -n \"$1: \"; cat \"$1\" 2>/dev/null && echo \" ($(cat \"${1%/temp}/type\" 2>/dev/null))\"' _ {} \\; 2>/dev/null || echo 'No thermal zones found'");
    
    std::cout << "\n3. lm-sensors output (if available):" << std::endl;
    system("sensors 2>/dev/null || echo 'lm-sensors not available'");
    
    std::cout << "\n4. ACPI thermal info:" << std::endl;
    system("find /proc/acpi -name 'thermal_zone' -type d 2>/dev/null | while read zone; do echo \"$zone:\"; cat \"$zone/temperature\" 2>/dev/null; done || echo 'No ACPI thermal zones'");
    
    std::cout << std::endl;
}

void debug_parsing_robustness() {
    std::cout << "=== PARSING ROBUSTNESS TESTING ===" << std::endl;
    
    // Test edge cases in /proc/stat parsing
    std::cout << "1. Testing CPU stats parsing:" << std::endl;
    system("head -1 /proc/stat");
    
    CPUStats stats = readCPUStatsOptimized();
    std::cout << "Parsed CPU stats - user:" << stats.user << " nice:" << stats.nice 
              << " system:" << stats.system << " idle:" << stats.idle << std::endl;
    std::cout << "Total: " << stats.getTotal() << " Idle: " << stats.getIdle() << std::endl;
    
    // Test /proc/meminfo parsing with different formats
    std::cout << "\n2. Testing memory parsing edge cases:" << std::endl;
    system("grep -E '(MemTotal|MemFree|MemAvailable|Buffers|Cached):' /proc/meminfo");
    
    // Test /proc/net/dev parsing
    std::cout << "\n3. Testing network parsing:" << std::endl;
    system("head -3 /proc/net/dev");
    
    // Check for any malformed interface lines
    auto interfaces = readNetworkInterfacesOptimized();
    std::cout << "Successfully parsed " << interfaces.size() << " network interfaces" << std::endl;
    
    for (const auto& iface : interfaces) {
        if (iface.name.empty()) {
            std::cout << "WARNING: Found interface with empty name!" << std::endl;
        }
        if (iface.rxBytes < 0 || iface.txBytes < 0) {
            std::cout << "WARNING: Negative byte counts for " << iface.name << std::endl;
        }
    }
    
    std::cout << "\n4. Testing process parsing edge cases:" << std::endl;
    system("ls /proc | grep -E '^[0-9]+$' | wc -l");
    
    auto processes = readProcessListOptimized();
    std::cout << "Successfully parsed " << processes.size() << " processes" << std::endl;
    
    // Check for any malformed process data
    int invalid_count = 0;
    for (const auto& proc : processes) {
        if (proc.pid <= 0 || proc.name.empty()) {
            invalid_count++;
        }
    }
    
    if (invalid_count > 0) {
        std::cout << "WARNING: Found " << invalid_count << " processes with invalid data" << std::endl;
    }
    
    std::cout << std::endl;
}

int main() {
    std::cout << "=== SYSTEM MONITOR DISCREPANCY DEBUGGING ===" << std::endl;
    std::cout << "Comparing system monitor output with Linux commands..." << std::endl << std::endl;
    
    debug_process_data();
    debug_memory_data();
    debug_network_data();
    debug_thermal_data();
    debug_parsing_robustness();
    
    std::cout << "=== DEBUGGING COMPLETE ===" << std::endl;
    std::cout << "Check output above for any discrepancies or parsing issues." << std::endl;
    
    return 0;
}
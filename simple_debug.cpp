#include "optimized_header.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstdlib>

// Forward declarations
CPUStats readCPUStatsOptimized();
MemoryInfo readMemoryInfoOptimized();
vector<NetworkInterfaceStats> readNetworkInterfacesOptimized();
vector<ThermalSensor> discoverThermalSensorsOptimized();
vector<ProcessInfo> readProcessListOptimized();
map<string, string> getInterfaceIPAddressesOptimized();
map<string, string> getInterfaceStatesOptimized();

void debug_process_data() {
    std::cout << "=== PROCESS DATA DEBUGGING ===" << std::endl;
    
    auto processes = readProcessListOptimized();
    std::cout << "System Monitor - Total processes: " << processes.size() << std::endl;
    
    std::cout << "\nRunning Linux commands for comparison:" << std::endl;
    std::cout << "ps aux | wc -l: ";
    std::cout.flush();
    system("ps aux | wc -l");
    
    std::cout << "ls /proc | grep -E '^[0-9]+$' | wc -l: ";
    std::cout.flush();
    system("ls /proc | grep -E '^[0-9]+$' | wc -l");
    
    // Show sample data
    std::cout << "\nSystem Monitor - Sample processes (first 5):" << std::endl;
    std::cout << std::setw(8) << "PID" << std::setw(16) << "Name" << std::setw(8) << "State" 
              << std::setw(12) << "RSS(KB)" << std::endl;
    
    for (int i = 0; i < std::min(5, (int)processes.size()); i++) {
        const auto& p = processes[i];
        std::cout << std::setw(8) << p.pid << std::setw(16) << p.name.substr(0,15) 
                  << std::setw(8) << p.state << std::setw(12) << p.rss << std::endl;
    }
    
    std::cout << "\nLinux ps command for comparison:" << std::endl;
    system("ps -eo pid,comm,state,rss --no-headers | head -5");
    std::cout << std::endl;
}

void debug_memory_data() {
    std::cout << "=== MEMORY DATA DEBUGGING ===" << std::endl;
    
    MemoryInfo memory = readMemoryInfoOptimized();
    
    std::cout << "System Monitor Memory Data (KB):" << std::endl;
    std::cout << "  Total:     " << memory.memTotal << std::endl;
    std::cout << "  Free:      " << memory.memFree << std::endl;
    std::cout << "  Available: " << memory.memAvailable << std::endl;
    std::cout << "  Used:      " << memory.memUsed << " (calculated)" << std::endl;
    std::cout << "  Buffers:   " << memory.buffers << std::endl;
    std::cout << "  Cached:    " << memory.cached << std::endl;
    std::cout << "  Used%:     " << std::fixed << std::setprecision(1) << memory.memUsedPercent << "%" << std::endl;
    
    std::cout << "\nLinux free -k command:" << std::endl;
    system("free -k");
    
    std::cout << "\nRaw /proc/meminfo key values:" << std::endl;
    system("grep -E '(MemTotal|MemFree|MemAvailable|Buffers|Cached|SwapTotal|SwapFree):' /proc/meminfo");
    std::cout << std::endl;
}

void debug_network_data() {
    std::cout << "=== NETWORK DATA DEBUGGING ===" << std::endl;
    
    auto interfaces = readNetworkInterfacesOptimized();
    
    std::cout << "System Monitor Network Data:" << std::endl;
    std::cout << std::setw(12) << "Interface" << std::setw(15) << "RX Bytes" << std::setw(15) << "TX Bytes" << std::endl;
    
    for (const auto& iface : interfaces) {
        std::cout << std::setw(12) << iface.name 
                  << std::setw(15) << iface.rxBytes 
                  << std::setw(15) << iface.txBytes << std::endl;
    }
    
    std::cout << "\nRaw /proc/net/dev:" << std::endl;
    system("cat /proc/net/dev");
    
    std::cout << "\nInterface states:" << std::endl;
    auto ip_addresses = getInterfaceIPAddressesOptimized();
    auto states = getInterfaceStatesOptimized();
    
    for (const auto& iface : interfaces) {
        std::cout << iface.name << ": ";
        auto state_it = states.find(iface.name);
        auto ip_it = ip_addresses.find(iface.name);
        
        if (state_it != states.end()) {
            std::cout << "State=" << state_it->second << " ";
        }
        if (ip_it != ip_addresses.end()) {
            std::cout << "IP=" << ip_it->second;
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void debug_thermal_data() {
    std::cout << "=== THERMAL DATA DEBUGGING ===" << std::endl;
    
    auto sensors = discoverThermalSensorsOptimized();
    
    std::cout << "System Monitor Thermal Data:" << std::endl;
    if (sensors.empty()) {
        std::cout << "  No thermal sensors detected" << std::endl;
    } else {
        for (const auto& sensor : sensors) {
            if (sensor.isValid) {
                std::cout << "  " << sensor.name << ": " << std::fixed << std::setprecision(1) 
                          << sensor.temperature << "°C (source: " << sensor.source << ")" << std::endl;
            }
        }
    }
    
    std::cout << "\nChecking thermal sources:" << std::endl;
    
    std::cout << "\n1. ThinkPad thermal:" << std::endl;
    system("cat /proc/acpi/ibm/thermal 2>/dev/null || echo 'Not available'");
    
    std::cout << "\n2. Thermal zones:" << std::endl;
    system("find /sys/class/thermal -name 'thermal_zone*/temp' 2>/dev/null | head -5 | while read f; do echo -n \"$f: \"; cat \"$f\" 2>/dev/null; done || echo 'No thermal zones'");
    
    std::cout << "\n3. Available thermal zone types:" << std::endl;
    system("find /sys/class/thermal -name 'thermal_zone*/type' 2>/dev/null | head -5 | while read f; do echo -n \"$(dirname \"$f\"): \"; cat \"$f\" 2>/dev/null; done");
    
    std::cout << std::endl;
}

void debug_cpu_data() {
    std::cout << "=== CPU DATA DEBUGGING ===" << std::endl;
    
    CPUStats stats = readCPUStatsOptimized();
    std::cout << "System Monitor CPU Stats:" << std::endl;
    std::cout << "  user: " << stats.user << ", nice: " << stats.nice << ", system: " << stats.system << std::endl;
    std::cout << "  idle: " << stats.idle << ", iowait: " << stats.iowait << ", irq: " << stats.irq << std::endl;
    std::cout << "  Total: " << stats.getTotal() << ", Idle: " << stats.getIdle() << std::endl;
    
    std::cout << "\nRaw /proc/stat first line:" << std::endl;
    system("head -1 /proc/stat");
    
    std::cout << std::endl;
}

int main() {
    std::cout << "=== SYSTEM MONITOR DISTRIBUTION COMPATIBILITY TEST ===" << std::endl;
    std::cout << "Testing cross-distribution compatibility and system capabilities..." << std::endl << std::endl;
    
    // Initialize system capability detection
    detectSystemCapabilities();
    std::cout << std::endl;
    
    debug_cpu_data();
    debug_memory_data();
    debug_network_data();
    debug_thermal_data();
    debug_process_data();
    
    std::cout << "=== COMPATIBILITY TEST COMPLETE ===" << std::endl;
    return 0;
}
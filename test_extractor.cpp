#include "header.h"
#include <iostream>
#include <fstream>
#include <sstream>

// Test utility to extract system information from our monitoring functions
// This allows us to test our functions against system commands

void printSystemInfo() {
    std::cout << "=== SYSTEM INFO ===" << std::endl;
    std::cout << "OS: " << getOsName() << std::endl;
    std::cout << "User: " << getCurrentUser() << std::endl;
    std::cout << "Hostname: " << getHostname() << std::endl;
    std::cout << "Total Processes: " << getTotalProcesses() << std::endl;
    std::cout << "CPU Model: " << getCPUModel() << std::endl;
}

void printMemoryInfo() {
    std::cout << "=== MEMORY INFO ===" << std::endl;
    MemoryInfo mem = readMemoryInfo();
    std::cout << "MemTotal: " << mem.memTotal << " kB" << std::endl;
    std::cout << "MemFree: " << mem.memFree << " kB" << std::endl;
    std::cout << "MemAvailable: " << mem.memAvailable << " kB" << std::endl;
    std::cout << "Buffers: " << mem.buffers << " kB" << std::endl;
    std::cout << "Cached: " << mem.cached << " kB" << std::endl;
    std::cout << "SwapTotal: " << mem.swapTotal << " kB" << std::endl;
    std::cout << "SwapFree: " << mem.swapFree << " kB" << std::endl;
    std::cout << "MemUsed: " << mem.memUsed << " kB" << std::endl;
    std::cout << "MemUsedPercent: " << mem.memUsedPercent << "%" << std::endl;
    std::cout << "SwapUsedPercent: " << mem.swapUsedPercent << "%" << std::endl;
}

void printDiskInfo() {
    std::cout << "=== DISK INFO ===" << std::endl;
    DiskInfo disk = readDiskInfo("/");
    std::cout << "Filesystem: " << disk.filesystem << std::endl;
    std::cout << "Mountpoint: " << disk.mountpoint << std::endl;
    std::cout << "Total: " << disk.total << " kB" << std::endl;
    std::cout << "Used: " << disk.used << " kB" << std::endl;
    std::cout << "Available: " << disk.available << " kB" << std::endl;
    std::cout << "UsedPercent: " << disk.usedPercent << "%" << std::endl;
}

void printNetworkInfo() {
    std::cout << "=== NETWORK INFO ===" << std::endl;
    vector<NetworkInterfaceStats> interfaces = readNetworkInterfaces();
    map<string, string> ipAddresses = getInterfaceIPAddresses();
    map<string, string> states = getInterfaceStates();
    
    for (const auto& iface : interfaces) {
        std::cout << "Interface: " << iface.name << std::endl;
        std::cout << "  RX Bytes: " << iface.rxBytes << std::endl;
        std::cout << "  TX Bytes: " << iface.txBytes << std::endl;
        std::cout << "  RX Packets: " << iface.rxPackets << std::endl;
        std::cout << "  TX Packets: " << iface.txPackets << std::endl;
        std::cout << "  RX Errors: " << iface.rxErrs << std::endl;
        std::cout << "  TX Errors: " << iface.txErrs << std::endl;
        
        if (ipAddresses.find(iface.name) != ipAddresses.end()) {
            std::cout << "  IP Address: " << ipAddresses[iface.name] << std::endl;
        }
        if (states.find(iface.name) != states.end()) {
            std::cout << "  State: " << states[iface.name] << std::endl;
        }
        std::cout << std::endl;
    }
}

void printThermalInfo() {
    std::cout << "=== THERMAL INFO ===" << std::endl;
    
    // Try ThinkPad thermal
    float thinkpadTemp = readThinkPadThermal();
    if (thinkpadTemp > 0) {
        std::cout << "ThinkPad Thermal: " << thinkpadTemp << "°C" << std::endl;
    }
    
    // Try thermal zones
    for (int i = 0; i < 10; i++) {
        float temp = readThermalZone(i);
        if (temp > 0) {
            std::cout << "Thermal Zone " << i << ": " << temp << "°C" << std::endl;
        }
    }
    
    // Try hwmon
    for (int i = 0; i < 10; i++) {
        string hwmonPath = "/sys/class/hwmon/hwmon" + std::to_string(i) + "/temp1_input";
        float temp = readHwmonTemp(hwmonPath);
        if (temp > 0) {
            std::cout << "Hwmon " << i << ": " << temp << "°C" << std::endl;
        }
    }
}

void printProcessInfo() {
    std::cout << "=== PROCESS INFO ===" << std::endl;
    vector<ProcessInfo> processes = readProcessList();
    
    // Sort by CPU usage
    std::sort(processes.begin(), processes.end(), 
              [](const ProcessInfo& a, const ProcessInfo& b) {
                  return a.cpuPercent > b.cpuPercent;
              });
    
    std::cout << "Total Processes: " << processes.size() << std::endl;
    std::cout << "Top 5 processes by CPU:" << std::endl;
    
    int count = 0;
    for (const auto& proc : processes) {
        if (count >= 5) break;
        std::cout << "  PID: " << proc.pid 
                  << " Name: " << proc.name 
                  << " CPU: " << proc.cpuPercent << "%" 
                  << " Mem: " << proc.memPercent << "%" 
                  << " State: " << proc.state << std::endl;
        count++;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_type>" << std::endl;
        std::cout << "test_types: system, memory, disk, network, thermal, processes, all" << std::endl;
        return 1;
    }
    
    string testType = argv[1];
    
    if (testType == "system" || testType == "all") {
        printSystemInfo();
    }
    if (testType == "memory" || testType == "all") {
        printMemoryInfo();
    }
    if (testType == "disk" || testType == "all") {
        printDiskInfo();
    }
    if (testType == "network" || testType == "all") {
        printNetworkInfo();
    }
    if (testType == "thermal" || testType == "all") {
        printThermalInfo();
    }
    if (testType == "processes" || testType == "all") {
        printProcessInfo();
    }
    
    return 0;
}
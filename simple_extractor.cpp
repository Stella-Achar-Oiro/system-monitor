#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <dirent.h>
#include <cctype>

using std::string;
using std::vector;
using std::map;

// Basic system info functions (copied from header.h without ImGui dependencies)
const char* getOsName() {
    static const char* osName = nullptr;
    if (osName == nullptr) {
        std::ifstream file("/etc/os-release");
        if (file.is_open()) {
            string line;
            while (std::getline(file, line)) {
                if (line.find("PRETTY_NAME=") == 0) {
                    size_t start = line.find('"');
                    size_t end = line.rfind('"');
                    if (start != string::npos && end != string::npos && start < end) {
                        static string osString = line.substr(start + 1, end - start - 1);
                        osName = osString.c_str();
                        break;
                    }
                }
            }
            file.close();
        }
        if (osName == nullptr) {
            osName = "Unknown";
        }
    }
    return osName;
}

string getCurrentUser() {
    char* user = getenv("USER");
    return user ? string(user) : "unknown";
}

string getHostname() {
    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, HOST_NAME_MAX) == 0) {
        return string(hostname);
    }
    return "unknown";
}

int getTotalProcesses() {
    int count = 0;
    DIR* procDir = opendir("/proc");
    if (procDir) {
        struct dirent* entry;
        while ((entry = readdir(procDir)) != nullptr) {
            if (isdigit(entry->d_name[0])) {
                count++;
            }
        }
        closedir(procDir);
    }
    return count;
}

struct MemoryInfo {
    long long memTotal, memFree, memAvailable, buffers, cached;
    long long swapTotal, swapFree, swapUsed;
    long long memUsed;
    float memUsedPercent, swapUsedPercent;
};

MemoryInfo readMemoryInfo() {
    MemoryInfo info = {};
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) return info;
    
    string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        string key;
        long long value;
        string unit;
        
        if (iss >> key >> value >> unit) {
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
    info.memUsed = info.memTotal - info.memFree;
    info.memUsedPercent = info.memTotal > 0 ? (float)info.memUsed / info.memTotal * 100.0f : 0.0f;
    info.swapUsedPercent = info.swapTotal > 0 ? (float)info.swapUsed / info.swapTotal * 100.0f : 0.0f;
    
    return info;
}

struct DiskInfo {
    string filesystem, mountpoint;
    long long total, used, available;
    float usedPercent;
};

DiskInfo readDiskInfo(const string& mountpoint) {
    DiskInfo info = {};
    info.mountpoint = mountpoint;
    
    struct statvfs stat;
    if (statvfs(mountpoint.c_str(), &stat) == 0) {
        info.total = (stat.f_blocks * stat.f_frsize) / 1024;
        info.available = (stat.f_bavail * stat.f_frsize) / 1024;
        info.used = info.total - info.available;
        info.usedPercent = info.total > 0 ? (float)info.used / info.total * 100.0f : 0.0f;
    }
    
    // Try to get filesystem name
    std::ifstream mounts("/proc/mounts");
    if (mounts.is_open()) {
        string line;
        while (std::getline(mounts, line)) {
            std::istringstream iss(line);
            string device, mount, fs;
            if (iss >> device >> mount >> fs) {
                if (mount == mountpoint) {
                    info.filesystem = device;
                    break;
                }
            }
        }
    }
    
    return info;
}

struct NetworkInterfaceStats {
    string name;
    long long rxBytes, rxPackets, rxErrs, rxDrop, rxFifo, rxFrame, rxCompressed, rxMulticast;
    long long txBytes, txPackets, txErrs, txDrop, txFifo, txColls, txCarrier, txCompressed;
    string ipAddress, state;
};

vector<NetworkInterfaceStats> readNetworkInterfaces() {
    vector<NetworkInterfaceStats> interfaces;
    std::ifstream file("/proc/net/dev");
    
    if (!file.is_open()) {
        return interfaces;
    }
    
    string line;
    int lineCount = 0;
    
    while (std::getline(file, line)) {
        lineCount++;
        if (lineCount <= 2) continue;
        
        size_t colonPos = line.find(':');
        if (colonPos == string::npos) continue;
        
        NetworkInterfaceStats interface;
        interface.name = line.substr(0, colonPos);
        
        // Trim whitespace from name
        size_t start = interface.name.find_first_not_of(" \t");
        size_t end = interface.name.find_last_not_of(" \t");
        if (start != string::npos && end != string::npos) {
            interface.name = interface.name.substr(start, end - start + 1);
        }
        
        string stats = line.substr(colonPos + 1);
        std::istringstream iss(stats);
        
        if (!(iss >> interface.rxBytes >> interface.rxPackets >> interface.rxErrs 
              >> interface.rxDrop >> interface.rxFifo >> interface.rxFrame 
              >> interface.rxCompressed >> interface.rxMulticast)) {
            continue;
        }
        
        if (!(iss >> interface.txBytes >> interface.txPackets >> interface.txErrs 
              >> interface.txDrop >> interface.txFifo >> interface.txColls 
              >> interface.txCarrier >> interface.txCompressed)) {
            continue;
        }
        
        interfaces.push_back(interface);
    }
    
    file.close();
    return interfaces;
}

map<string, string> getInterfaceIPAddresses() {
    map<string, string> ipAddresses;
    struct ifaddrs *ifaddr, *ifa;
    
    if (getifaddrs(&ifaddr) == -1) {
        return ipAddresses;
    }
    
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* addr_in = (struct sockaddr_in*)ifa->ifa_addr;
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(addr_in->sin_addr), ip_str, INET_ADDRSTRLEN);
            ipAddresses[ifa->ifa_name] = string(ip_str);
        }
    }
    
    freeifaddrs(ifaddr);
    return ipAddresses;
}

map<string, string> getInterfaceStates() {
    map<string, string> states;
    struct ifaddrs *ifaddr, *ifa;
    
    if (getifaddrs(&ifaddr) == -1) {
        return states;
    }
    
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        
        string state = "DOWN";
        if (ifa->ifa_flags & IFF_UP) {
            state = "UP";
        }
        
        if (states.find(ifa->ifa_name) == states.end() || state == "UP") {
            states[ifa->ifa_name] = state;
        }
    }
    
    freeifaddrs(ifaddr);
    return states;
}

float readThinkPadThermal() {
    std::ifstream file("/proc/acpi/ibm/thermal");
    if (file.is_open()) {
        string line;
        if (std::getline(file, line)) {
            std::istringstream iss(line);
            string word;
            while (iss >> word) {
                if (word.find("temperatures:") != string::npos) {
                    float temp;
                    if (iss >> temp) {
                        return temp;
                    }
                }
            }
        }
    }
    return -1.0f;
}

// Test functions
void printSystemInfo() {
    std::cout << "=== SYSTEM INFO ===" << std::endl;
    std::cout << "OS: " << getOsName() << std::endl;
    std::cout << "User: " << getCurrentUser() << std::endl;
    std::cout << "Hostname: " << getHostname() << std::endl;
    std::cout << "Total Processes: " << getTotalProcesses() << std::endl;
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
    
    float thinkpadTemp = readThinkPadThermal();
    if (thinkpadTemp > 0) {
        std::cout << "ThinkPad Thermal: " << thinkpadTemp << "°C" << std::endl;
    } else {
        std::cout << "ThinkPad Thermal: Not available" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_type>" << std::endl;
        std::cout << "test_types: system, memory, disk, network, thermal, all" << std::endl;
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
    
    return 0;
}
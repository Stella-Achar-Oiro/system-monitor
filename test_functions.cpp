#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <pwd.h>
#include <cstring>
#include <cstdlib>
#include <cctype>

using namespace std;

// Copy the structures and function declarations we need
struct TaskCounts {
    int total, running, sleeping, stopped, zombie;
};

struct MemInfo {
    long long total, available, used;
};

struct DiskInfo {
    long long total, used, available;
};

struct IP4 {
    char *name;
    char addressBuffer[INET_ADDRSTRLEN];
};

struct Networks {
    vector<IP4> ip4s;
};

struct RX {
    long long bytes;
    long long packets;
    long long errs;
    long long drop;
    long long fifo;
    long long frame;
    long long compressed;
    long long multicast;
};

struct TX {
    long long bytes;
    long long packets;
    long long errs;
    long long drop;
    long long fifo;
    long long colls;
    long long carrier;
    long long compressed;
};

struct NetStats {
    map<string, RX> rx;
    map<string, TX> tx;
};

struct FanInfo {
    bool enabled;
    int speed;
    int level;
};

// Function declarations
string CPUinfo();
const char *getOsName();
string getUsername();
string getHostname();
TaskCounts getTaskCounts();
float getThermalTemp();
FanInfo getFanInfo();
MemInfo getMemInfo();
MemInfo getSwapInfo();
DiskInfo getDiskInfo();
Networks getNetworks();
NetStats getNetStats();
string formatBytes(long long bytes);

int main() {
    std::cout << "=== System Monitor Function Tests ===" << std::endl;
    
    // Test OS name
    std::cout << "OS Name: " << getOsName() << std::endl;
    
    // Test username
    std::cout << "Username: " << getUsername() << std::endl;
    
    // Test hostname
    std::cout << "Hostname: " << getHostname() << std::endl;
    
    // Test CPU info
    std::cout << "CPU: " << CPUinfo() << std::endl;
    
    // Test task counts
    TaskCounts tasks = getTaskCounts();
    std::cout << "Tasks - Total: " << tasks.total << ", Running: " << tasks.running 
              << ", Sleeping: " << tasks.sleeping << ", Stopped: " << tasks.stopped 
              << ", Zombie: " << tasks.zombie << std::endl;
    
    // Test memory info
    MemInfo ram = getMemInfo();
    std::cout << "RAM - Total: " << formatBytes(ram.total) << ", Used: " << formatBytes(ram.used) 
              << ", Available: " << formatBytes(ram.available) << std::endl;
    
    MemInfo swap = getSwapInfo();
    std::cout << "SWAP - Total: " << formatBytes(swap.total) << ", Used: " << formatBytes(swap.used) 
              << ", Available: " << formatBytes(swap.available) << std::endl;
    
    // Test disk info
    DiskInfo disk = getDiskInfo();
    std::cout << "Disk - Total: " << formatBytes(disk.total) << ", Used: " << formatBytes(disk.used) 
              << ", Available: " << formatBytes(disk.available) << std::endl;
    
    // Test network info
    Networks networks = getNetworks();
    std::cout << "Network Interfaces:" << std::endl;
    for (const auto& ip4 : networks.ip4s) {
        std::cout << "  " << ip4.name << ": " << ip4.addressBuffer << std::endl;
    }
    
    // Test network stats
    NetStats stats = getNetStats();
    std::cout << "Network Stats:" << std::endl;
    for (const auto& pair : stats.rx) {
        std::cout << "  " << pair.first << " RX: " << formatBytes(pair.second.bytes) << std::endl;
    }
    for (const auto& pair : stats.tx) {
        std::cout << "  " << pair.first << " TX: " << formatBytes(pair.second.bytes) << std::endl;
    }
    
    // Test thermal
    float temp = getThermalTemp();
    std::cout << "Thermal Temperature: " << temp << "°C" << std::endl;
    
    // Test fan
    FanInfo fan = getFanInfo();
    std::cout << "Fan - Enabled: " << (fan.enabled ? "Yes" : "No") 
              << ", Speed: " << fan.speed << " RPM, Level: " << fan.level << std::endl;
    
    return 0;
}
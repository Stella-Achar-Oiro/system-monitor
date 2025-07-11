#include "header.h"

// read network interface statistics from /proc/net/dev
vector<NetworkStats> readNetworkStats()
{
    vector<NetworkStats> stats;
    ifstream file("/proc/net/dev");
    if (file.is_open())
    {
        string line;
        // Skip header lines
        getline(file, line);
        getline(file, line);
        
        while (getline(file, line))
        {
            istringstream iss(line);
            string interface;
            iss >> interface;
            
            // Remove colon from interface name
            if (interface.back() == ':')
                interface.pop_back();
            
            NetworkStats netStat;
            netStat.interface = interface;
            
            iss >> netStat.rxBytes >> netStat.rxPackets >> netStat.rxErrs >> netStat.rxDrop
                >> netStat.rxFifo >> netStat.rxFrame >> netStat.rxCompressed >> netStat.rxMulticast
                >> netStat.txBytes >> netStat.txPackets >> netStat.txErrs >> netStat.txDrop
                >> netStat.txFifo >> netStat.txColls >> netStat.txCarrier >> netStat.txCompressed;
            
            stats.push_back(netStat);
        }
        file.close();
    }
    return stats;
}

// get network interface IPv4 addresses
Networks getNetworkInterfaces()
{
    Networks networks;
    struct ifaddrs *ifaddr, *ifa;
    
    if (getifaddrs(&ifaddr) == -1)
        return networks;
    
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;
        
        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            IP4 ip4;
            ip4.name = strdup(ifa->ifa_name);
            
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &(addr->sin_addr), ip4.addressBuffer, INET_ADDRSTRLEN);
            
            networks.ip4s.push_back(ip4);
        }
    }
    
    freeifaddrs(ifaddr);
    return networks;
}

// format network speed to human readable format
string formatNetworkSpeed(long long bytes)
{
    const char* units[] = {"B/s", "KB/s", "MB/s", "GB/s"};
    int unit = 0;
    double speed = bytes;
    
    while (speed >= 1024 && unit < 3)
    {
        speed /= 1024;
        unit++;
    }
    
    char buffer[64];
    if (unit == 0)
        snprintf(buffer, sizeof(buffer), "%.0f %s", speed, units[unit]);
    else
        snprintf(buffer, sizeof(buffer), "%.2f %s", speed, units[unit]);
    
    return string(buffer);
}

// convert bytes to appropriate unit (B, KB, MB, GB)
string formatNetworkBytes(long long bytes)
{
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = bytes;
    
    // Auto-adjust to appropriate unit
    while (size >= 1024 && unit < 4)
    {
        size /= 1024;
        unit++;
    }
    
    char buffer[64];
    if (unit == 0)
        snprintf(buffer, sizeof(buffer), "%.0f %s", size, units[unit]);
    else if (size < 10)
        snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unit]);
    else if (size < 100)
        snprintf(buffer, sizeof(buffer), "%.1f %s", size, units[unit]);
    else
        snprintf(buffer, sizeof(buffer), "%.0f %s", size, units[unit]);
    
    return string(buffer);
}

// get RX statistics for a specific interface
RX getRXStats(const string& interface)
{
    RX rx = {0};
    vector<NetworkStats> stats = readNetworkStats();
    
    for (const auto& stat : stats)
    {
        if (stat.interface == interface)
        {
            rx.bytes = stat.rxBytes;
            rx.packets = stat.rxPackets;
            rx.errs = stat.rxErrs;
            rx.drop = stat.rxDrop;
            rx.fifo = stat.rxFifo;
            rx.frame = stat.rxFrame;
            rx.compressed = stat.rxCompressed;
            rx.multicast = stat.rxMulticast;
            break;
        }
    }
    
    return rx;
}

// get TX statistics for a specific interface
TX getTXStats(const string& interface)
{
    TX tx = {0};
    vector<NetworkStats> stats = readNetworkStats();
    
    for (const auto& stat : stats)
    {
        if (stat.interface == interface)
        {
            tx.bytes = stat.txBytes;
            tx.packets = stat.txPackets;
            tx.errs = stat.txErrs;
            tx.drop = stat.txDrop;
            tx.fifo = stat.txFifo;
            tx.colls = stat.txColls;
            tx.carrier = stat.txCarrier;
            tx.compressed = stat.txCompressed;
            break;
        }
    }
    
    return tx;
}

// get list of all network interfaces
vector<string> getNetworkInterfaceList()
{
    vector<string> interfaces;
    vector<NetworkStats> stats = readNetworkStats();
    
    for (const auto& stat : stats)
    {
        interfaces.push_back(stat.interface);
    }
    
    return interfaces;
}

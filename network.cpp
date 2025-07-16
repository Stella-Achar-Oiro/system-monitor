#include "header.h"
#include <cstring>
#include <cstdio>

Networks getNetworks()
{
    Networks networks;
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
        return networks;
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            IP4 ip4;
            ip4.name = strdup(ifa->ifa_name);
            struct sockaddr_in* addr_in = (struct sockaddr_in*)ifa->ifa_addr;
            inet_ntop(AF_INET, &(addr_in->sin_addr), ip4.addressBuffer, INET_ADDRSTRLEN);
            networks.ip4s.push_back(ip4);
        }
    }

    freeifaddrs(ifaddr);
    return networks;
}

NetStats getNetStats()
{
    NetStats stats;
    ifstream file("/proc/net/dev");
    string line;

    getline(file, line);
    getline(file, line);

    while (getline(file, line)) {
        size_t colon = line.find(':');
        if (colon != string::npos) {
            string iface = line.substr(0, colon);
            iface.erase(0, iface.find_first_not_of(" \t"));

            string data = line.substr(colon + 1);

            RX rx = {0};
            TX tx = {0};

            sscanf(data.c_str(), "%lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld",
                   &rx.bytes, &rx.packets, &rx.errs, &rx.drop, &rx.fifo, &rx.frame, &rx.compressed, &rx.multicast,
                   &tx.bytes, &tx.packets, &tx.errs, &tx.drop, &tx.fifo, &tx.colls, &tx.carrier, &tx.compressed);

            stats.rx[iface] = rx;
            stats.tx[iface] = tx;
        }
    }

    return stats;
}

string formatBytes(long long bytes)
{
    const char* units[] = {"B", "K", "M", "G"};
    int unit = 0;
    double size = (double)bytes;

    while (size >= 1024.0 && unit < 3) {
        size /= 1024.0;
        unit++;
    }

    char buffer[64];
    if (unit == 0) {
        snprintf(buffer, sizeof(buffer), "%.0f%s", size, units[unit]);
    } else {
        snprintf(buffer, sizeof(buffer), "%.0f%s", size, units[unit]);
    }
    return string(buffer);
}

// To make sure you don't declare the function more than once by including the header multiple times.
#ifndef header_H
#define header_H

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <dirent.h>
#include <vector>
#include <iostream>
#include <cmath>
// lib to read from file
#include <fstream>
#include <sstream>
// for the name of the computer and the logged in user
#include <unistd.h>
#include <limits.h>
// this is for us to get the cpu information
// mostly in unix system
// not sure if it will work in windows
#include <cpuid.h>
// this is for the memory usage and other memory visualization
// for linux gotta find a way for windows
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
// for time and date
#include <ctime>
// ifconfig ip addresses
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <algorithm>

using namespace std;

struct CPUStats
{
    long long int user;
    long long int nice;
    long long int system;
    long long int idle;
    long long int iowait;
    long long int irq;
    long long int softirq;
    long long int steal;
    long long int guest;
    long long int guestNice;
};

// processes `stat`
struct Proc
{
    int pid;
    string name;
    char state;
    long long int vsize;
    long long int rss;
    long long int utime;
    long long int stime;
};

struct IP4
{
    char *name;
    char addressBuffer[INET_ADDRSTRLEN];
};

struct Networks
{
    vector<IP4> ip4s;
};

struct TX
{
    int bytes;
    int packets;
    int errs;
    int drop;
    int fifo;
    int frame;
    int compressed;
    int multicast;
};

struct RX
{
    int bytes;
    int packets;
    int errs;
    int drop;
    int fifo;
    int colls;
    int carrier;
    int compressed;
};

// system stats
string CPUinfo();
const char *getOsName();
string getCurrentUser();
string getHostname();
int getTotalProcesses();
CPUStats readCPUStats();
float calculateCPUPercent(const CPUStats &current, const CPUStats &previous);
float readThermalTemp();

// memory and processes
struct MemoryInfo
{
    long long total;
    long long free;
    long long available;
    long long used;
    long long cached;
    long long buffers;
    long long swapTotal;
    long long swapFree;
    long long swapUsed;
    float memUsedPercent;
    float swapUsedPercent;
};

struct DiskInfo
{
    string filesystem;
    string mountpoint;
    long long total;
    long long used;
    long long free;
    float usedPercent;
};

MemoryInfo readMemoryInfo();
vector<Proc> readProcessList();
vector<DiskInfo> readDiskInfo();
string formatBytes(long long bytes);
float calculateProcessCPU(const Proc& current, const Proc& previous, float deltaTime);

// network
struct NetworkStats
{
    string interface;
    long long rxBytes, rxPackets, rxErrs, rxDrop, rxFifo, rxFrame, rxCompressed, rxMulticast;
    long long txBytes, txPackets, txErrs, txDrop, txFifo, txColls, txCarrier, txCompressed;
};

vector<NetworkStats> readNetworkStats();
Networks getNetworkInterfaces();
string formatNetworkSpeed(long long bytes);
string formatNetworkBytes(long long bytes);
RX getRXStats(const string& interface);
TX getTXStats(const string& interface);
vector<string> getNetworkInterfaceList();

#endif

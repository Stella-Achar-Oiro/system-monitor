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
    float cpu_percent;
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

struct RX
{
    long long bytes;
    long long packets;
    long long errs;
    long long drop;
    long long fifo;
    long long frame;
    long long compressed;
    long long multicast;
};

struct TX
{
    long long bytes;
    long long packets;
    long long errs;
    long long drop;
    long long fifo;
    long long colls;
    long long carrier;
    long long compressed;
};

// System stats
string CPUinfo();
const char *getOsName();
string getUsername();
string getHostname();
struct TaskCounts {
    int total, running, sleeping, stopped, zombie;
};
TaskCounts getTaskCounts();
CPUStats getCPUStats();
float calculateCPUUsage();
float getThermalTemp();
struct FanInfo {
    bool enabled;
    int speed;
    int level;
};
FanInfo getFanInfo();

// Memory and processes
vector<Proc> getProcesses();
struct MemInfo {
    long long total, available, used;
};
MemInfo getMemInfo();
MemInfo getSwapInfo();
struct DiskInfo {
    long long total, used, available;
};
DiskInfo getDiskInfo();

// Network
Networks getNetworks();
struct NetStats {
    map<string, RX> rx;
    map<string, TX> tx;
};
NetStats getNetStats();
string formatBytes(long long bytes);

#endif

#include "header.h"
#include <cstdlib>
#include <cctype>

vector<Proc> getProcesses()
{
    vector<Proc> processes;
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) return processes;

    // Get system uptime for CPU calculation
    ifstream uptime_file("/proc/uptime");
    double uptime = 0.0;
    if (uptime_file.is_open()) {
        uptime_file >> uptime;
    }

    // Get system clock ticks per second
    long clock_ticks = sysconf(_SC_CLK_TCK);

    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        if (isdigit(entry->d_name[0])) {
            Proc proc = {0};
            proc.pid = atoi(entry->d_name);

            string stat_path = "/proc/" + string(entry->d_name) + "/stat";
            ifstream stat_file(stat_path);
            if (stat_file.is_open()) {
                string line;
                getline(stat_file, line);
                if (!line.empty()) {
                    char name[256];
                    long long starttime;
                    sscanf(line.c_str(), "%d %s %c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lld %lld %*d %*d %*d %*d %*d %*d %lld %lld %lld",
                           &proc.pid, name, &proc.state, &proc.utime, &proc.stime, &starttime, &proc.vsize, &proc.rss);
                    proc.name = string(name);
                    if (proc.name.front() == '(' && proc.name.back() == ')') {
                        proc.name = proc.name.substr(1, proc.name.length() - 2);
                    }

                    // Calculate CPU percentage
                    double total_time = (double)(proc.utime + proc.stime) / clock_ticks;
                    double seconds = uptime - (double)starttime / clock_ticks;
                    if (seconds > 0) {
                        proc.cpu_percent = (float)(100.0 * total_time / seconds);
                    } else {
                        proc.cpu_percent = 0.0f;
                    }

                    processes.push_back(proc);
                }
            }
        }
    }
    closedir(proc_dir);
    return processes;
}

MemInfo getMemInfo()
{
    MemInfo info = {0, 0, 0};
    ifstream file("/proc/meminfo");
    string line;
    long long free = 0;
    while (getline(file, line)) {
        if (line.find("MemTotal:") != string::npos) {
            sscanf(line.c_str(), "MemTotal: %lld kB", &info.total);
            info.total *= 1024;
        } else if (line.find("MemFree:") != string::npos) {
            sscanf(line.c_str(), "MemFree: %lld kB", &free);
        } else if (line.find("MemAvailable:") != string::npos) {
            sscanf(line.c_str(), "MemAvailable: %lld kB", &info.available);
            info.available *= 1024;
        }
    }
    info.used = info.total - info.available;
    return info;
}

MemInfo getSwapInfo()
{
    MemInfo info = {0, 0, 0};
    ifstream file("/proc/meminfo");
    string line;
    while (getline(file, line)) {
        if (line.find("SwapTotal:") != string::npos) {
            sscanf(line.c_str(), "SwapTotal: %lld kB", &info.total);
            info.total *= 1024;
        } else if (line.find("SwapFree:") != string::npos) {
            long long free;
            sscanf(line.c_str(), "SwapFree: %lld kB", &free);
            info.available = free * 1024;
        }
    }
    info.used = info.total - info.available;
    return info;
}

DiskInfo getDiskInfo()
{
    DiskInfo info = {0, 0, 0};
    struct statvfs stat;
    if (statvfs("/", &stat) == 0) {
        info.total = stat.f_blocks * stat.f_frsize;
        info.available = stat.f_bavail * stat.f_frsize;
        info.used = info.total - (stat.f_bfree * stat.f_frsize);
    }
    return info;
}

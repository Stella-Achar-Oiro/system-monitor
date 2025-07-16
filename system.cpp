#include "header.h"
#include <cstdlib>
#include <cstring>
#include <pwd.h>
#include <sys/types.h>

string CPUinfo()
{
    ifstream file("/proc/cpuinfo");
    string line;
    while (getline(file, line)) {
        if (line.find("model name") != string::npos) {
            size_t pos = line.find(":");
            if (pos != string::npos) {
                return line.substr(pos + 2);
            }
        }
    }
    return "Unknown CPU";
}

const char *getOsName()
{
#ifdef _WIN32
    return "Windows 32-bit";
#elif _WIN64
    return "Windows 64-bit";
#elif __APPLE__ || __MACH__
    return "Mac OSX";
#elif __linux__
    return "Linux";
#elif __FreeBSD__
    return "FreeBSD";
#elif __unix || __unix__
    return "Unix";
#else
    return "Other";
#endif
}

string getUsername()
{
    char* user = getenv("USER");
    if (user && strlen(user) > 0) {
        return string(user);
    }
    
    // Fallback to getlogin() or getpwuid()
    char* login = getlogin();
    if (login) {
        return string(login);
    }
    
    // Final fallback to uid lookup
    uid_t uid = getuid();
    struct passwd* pw = getpwuid(uid);
    if (pw) {
        return string(pw->pw_name);
    }
    
    return "Unknown";
}

string getHostname()
{
    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, HOST_NAME_MAX) == 0) {
        return string(hostname);
    }
    return "Unknown";
}

TaskCounts getTaskCounts()
{
    TaskCounts counts = {0, 0, 0, 0, 0};
    
    DIR* proc_dir = opendir("/proc");
    if (proc_dir) {
        struct dirent* entry;
        while ((entry = readdir(proc_dir)) != nullptr) {
            if (isdigit(entry->d_name[0])) {
                string stat_path = "/proc/" + string(entry->d_name) + "/stat";
                ifstream stat_file(stat_path);
                if (stat_file.is_open()) {
                    string line;
                    getline(stat_file, line);
                    if (!line.empty()) {
                        char state;
                        sscanf(line.c_str(), "%*d %*s %c", &state);
                        counts.total++;
                        switch (state) {
                            case 'R': counts.running++; break;
                            case 'S': case 'D': counts.sleeping++; break;
                            case 'T': case 't': counts.stopped++; break;
                            case 'Z': counts.zombie++; break;
                        }
                    }
                }
            }
        }
        closedir(proc_dir);
    }
    return counts;
}

CPUStats getCPUStats()
{
    CPUStats stats = {0};
    ifstream file("/proc/stat");
    string line;
    if (getline(file, line) && line.find("cpu") == 0) {
        sscanf(line.c_str(), "cpu %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld",
               &stats.user, &stats.nice, &stats.system, &stats.idle,
               &stats.iowait, &stats.irq, &stats.softirq, &stats.steal,
               &stats.guest, &stats.guestNice);
    }
    return stats;
}

float calculateCPUUsage()
{
    static CPUStats prev = {0};
    CPUStats curr = getCPUStats();
    
    long long prevIdle = prev.idle + prev.iowait;
    long long currIdle = curr.idle + curr.iowait;
    
    long long prevNonIdle = prev.user + prev.nice + prev.system + prev.irq + prev.softirq + prev.steal;
    long long currNonIdle = curr.user + curr.nice + curr.system + curr.irq + curr.softirq + curr.steal;
    
    long long prevTotal = prevIdle + prevNonIdle;
    long long currTotal = currIdle + currNonIdle;
    
    long long totalDiff = currTotal - prevTotal;
    long long idleDiff = currIdle - prevIdle;
    
    float usage = 0.0f;
    if (totalDiff > 0) {
        usage = (float)(totalDiff - idleDiff) / totalDiff * 100.0f;
    }
    
    prev = curr;
    return usage;
}

float getThermalTemp()
{
    ifstream file("/proc/acpi/ibm/thermal");
    if (file.is_open()) {
        string line;
        getline(file, line);
        if (line.find("temperatures:") != string::npos) {
            int temp;
            if (sscanf(line.c_str(), "temperatures: %d", &temp) == 1) {
                return (float)temp;
            }
        }
    }
    
    ifstream hwmon("/sys/class/thermal/thermal_zone0/temp");
    if (hwmon.is_open()) {
        int temp;
        hwmon >> temp;
        return temp / 1000.0f;
    }
    
    return 0.0f;
}

FanInfo getFanInfo()
{
    FanInfo info = {false, 0, 0};
    ifstream file("/proc/acpi/ibm/fan");
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            if (line.find("status:") != string::npos) {
                info.enabled = line.find("enabled") != string::npos;
            } else if (line.find("speed:") != string::npos) {
                sscanf(line.c_str(), "speed: %d", &info.speed);
            } else if (line.find("level:") != string::npos) {
                sscanf(line.c_str(), "level: %d", &info.level);
            }
        }
    }
    return info;
}

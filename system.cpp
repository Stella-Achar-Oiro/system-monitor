#include "header.h"

// get cpu id and information, you can use `proc/cpuinfo`
string CPUinfo()
{
    char CPUBrandString[0x40];
    unsigned int CPUInfo[4] = {0, 0, 0, 0};

    // unix system
    // for windoes maybe we must add the following
    // __cpuid(regs, 0);
    // regs is the array of 4 positions
    __cpuid(0x80000000, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
    unsigned int nExIds = CPUInfo[0];

    memset(CPUBrandString, 0, sizeof(CPUBrandString));

    for (unsigned int i = 0x80000000; i <= nExIds; ++i)
    {
        __cpuid(i, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);

        if (i == 0x80000002)
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000003)
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000004)
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
    }
    string str(CPUBrandString);
    return str;
}

// getOsName, this will get the OS of the current computer
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

// get current user name
string getCurrentUser()
{
    char *username = getenv("USER");
    if (username != NULL)
        return string(username);
    else
        return "Unknown";
}

// get computer hostname
string getHostname()
{
    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, HOST_NAME_MAX) == 0)
        return string(hostname);
    else
        return "Unknown";
}

// get total number of processes
int getTotalProcesses()
{
    DIR *procDir = opendir("/proc");
    if (procDir == NULL)
        return 0;

    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(procDir)) != NULL)
    {
        // Check if the directory name is a number (PID)
        if (isdigit(entry->d_name[0]))
            count++;
    }
    closedir(procDir);
    return count;
}

// read CPU statistics from /proc/stat
CPUStats readCPUStats()
{
    CPUStats stats = {0};
    ifstream file("/proc/stat");
    if (file.is_open())
    {
        string line;
        if (getline(file, line))
        {
            istringstream iss(line);
            string cpu;
            iss >> cpu >> stats.user >> stats.nice >> stats.system >> stats.idle 
                >> stats.iowait >> stats.irq >> stats.softirq >> stats.steal 
                >> stats.guest >> stats.guestNice;
        }
        file.close();
    }
    return stats;
}

// calculate CPU usage percentage
float calculateCPUPercent(const CPUStats &current, const CPUStats &previous)
{
    long long int prevIdle = previous.idle + previous.iowait;
    long long int idle = current.idle + current.iowait;

    long long int prevNonIdle = previous.user + previous.nice + previous.system + 
                                previous.irq + previous.softirq + previous.steal;
    long long int nonIdle = current.user + current.nice + current.system + 
                           current.irq + current.softirq + current.steal;

    long long int prevTotal = prevIdle + prevNonIdle;
    long long int total = idle + nonIdle;

    long long int totalDiff = total - prevTotal;
    long long int idleDiff = idle - prevIdle;

    if (totalDiff == 0)
        return 0.0f;

    return (float)(totalDiff - idleDiff) / totalDiff * 100.0f;
}

// read thermal information
float readThermalTemp()
{
    ifstream file("/sys/class/thermal/thermal_zone0/temp");
    if (file.is_open())
    {
        int temp;
        file >> temp;
        file.close();
        return temp / 1000.0f; // Convert from millidegrees to degrees
    }
    return -1.0f; // Error reading temperature
}

// read fan information (simplified)
struct FanInfo
{
    bool enabled;
    int speed;
    int level;
};

FanInfo readFanInfo()
{
    FanInfo fan = {false, 0, 0};
    
    // Try to read fan information from hwmon
    for (int i = 0; i < 10; i++)
    {
        string fanPath = "/sys/class/hwmon/hwmon" + to_string(i) + "/fan1_input";
        ifstream file(fanPath);
        if (file.is_open())
        {
            fan.enabled = true;
            file >> fan.speed;
            file.close();
            fan.level = fan.speed / 1000; // Simplified level calculation
            break;
        }
    }
    
    return fan;
}

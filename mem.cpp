#include "header.h"

// read memory information from /proc/meminfo
MemoryInfo readMemoryInfo()
{
    MemoryInfo mem = {0};
    ifstream file("/proc/meminfo");
    if (file.is_open())
    {
        string line;
        while (getline(file, line))
        {
            if (line.find("MemTotal:") == 0)
                mem.total = stoll(line.substr(9)) * 1024; // Convert KB to bytes
            else if (line.find("MemFree:") == 0)
                mem.free = stoll(line.substr(8)) * 1024;
            else if (line.find("MemAvailable:") == 0)
                mem.available = stoll(line.substr(13)) * 1024;
            else if (line.find("Cached:") == 0)
                mem.cached = stoll(line.substr(7)) * 1024;
            else if (line.find("Buffers:") == 0)
                mem.buffers = stoll(line.substr(8)) * 1024;
            else if (line.find("SwapTotal:") == 0)
                mem.swapTotal = stoll(line.substr(10)) * 1024;
            else if (line.find("SwapFree:") == 0)
                mem.swapFree = stoll(line.substr(9)) * 1024;
        }
        file.close();
    }
    
    // Calculate derived values
    mem.used = mem.total - mem.available;
    mem.swapUsed = mem.swapTotal - mem.swapFree;
    mem.memUsedPercent = (float)mem.used / mem.total * 100.0f;
    if (mem.swapTotal > 0)
        mem.swapUsedPercent = (float)mem.swapUsed / mem.swapTotal * 100.0f;
    
    return mem;
}

// read process information from /proc/[pid]/stat
vector<Proc> readProcessList()
{
    vector<Proc> processes;
    DIR *procDir = opendir("/proc");
    if (procDir == NULL)
        return processes;

    struct dirent *entry;
    while ((entry = readdir(procDir)) != NULL)
    {
        if (isdigit(entry->d_name[0]))
        {
            int pid = atoi(entry->d_name);
            string statPath = "/proc/" + string(entry->d_name) + "/stat";
            ifstream file(statPath);
            if (file.is_open())
            {
                Proc proc;
                proc.pid = pid;
                
                string line;
                if (getline(file, line))
                {
                    istringstream iss(line);
                    string field;
                    int fieldIndex = 0;
                    
                    while (iss >> field)
                    {
                        if (fieldIndex == 1) // Process name
                        {
                            proc.name = field;
                            // Remove parentheses
                            if (proc.name.front() == '(')
                                proc.name.erase(0, 1);
                            if (proc.name.back() == ')')
                                proc.name.pop_back();
                        }
                        else if (fieldIndex == 2) // State
                            proc.state = field[0];
                        else if (fieldIndex == 13) // User time
                            proc.utime = stoll(field);
                        else if (fieldIndex == 14) // System time
                            proc.stime = stoll(field);
                        else if (fieldIndex == 22) // Virtual memory size
                            proc.vsize = stoll(field);
                        else if (fieldIndex == 23) // Resident set size
                            proc.rss = stoll(field);
                        
                        fieldIndex++;
                        if (fieldIndex > 23) break;
                    }
                }
                file.close();
                processes.push_back(proc);
            }
        }
    }
    closedir(procDir);
    return processes;
}

// read disk information using statvfs
vector<DiskInfo> readDiskInfo()
{
    vector<DiskInfo> disks;
    
    // Read mounted filesystems from /proc/mounts
    ifstream file("/proc/mounts");
    if (file.is_open())
    {
        string line;
        while (getline(file, line))
        {
            istringstream iss(line);
            string device, mountpoint, fstype;
            iss >> device >> mountpoint >> fstype;
            
            // Skip special filesystems
            if (device.find("/dev/") == 0 || mountpoint == "/")
            {
                struct statvfs stat;
                if (statvfs(mountpoint.c_str(), &stat) == 0)
                {
                    DiskInfo disk;
                    disk.filesystem = device;
                    disk.mountpoint = mountpoint;
                    disk.total = stat.f_blocks * stat.f_frsize;
                    disk.free = stat.f_bavail * stat.f_frsize;
                    disk.used = disk.total - disk.free;
                    disk.usedPercent = (float)disk.used / disk.total * 100.0f;
                    disks.push_back(disk);
                }
            }
        }
        file.close();
    }
    
    return disks;
}

// format bytes to human readable format
string formatBytes(long long bytes)
{
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = bytes;
    
    while (size >= 1024 && unit < 4)
    {
        size /= 1024;
        unit++;
    }
    
    char buffer[64];
    if (unit == 0)
        snprintf(buffer, sizeof(buffer), "%.0f %s", size, units[unit]);
    else
        snprintf(buffer, sizeof(buffer), "%.1f %s", size, units[unit]);
    
    return string(buffer);
}

// calculate process CPU usage
float calculateProcessCPU(const Proc& current, const Proc& previous, float deltaTime)
{
    if (deltaTime <= 0) return 0.0f;
    
    long long totalTime = (current.utime + current.stime) - (previous.utime + previous.stime);
    float cpuPercent = (float)totalTime / (deltaTime * sysconf(_SC_CLK_TCK)) * 100.0f;
    
    return cpuPercent;
}

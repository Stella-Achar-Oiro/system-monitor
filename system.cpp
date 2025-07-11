#include "header.h"

// get cpu id and information, you can use `proc/cpuinfo`
string CPUinfo()
{
    char CPUBrandString[0x40];
    unsigned int CPUInfo[4] = {0, 0, 0, 0};

    // Check if extended CPUID is supported
    __cpuid(0x80000000, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
    unsigned int nExIds = CPUInfo[0];
    
    // Validate that extended CPUID is available
    if (nExIds < 0x80000004) {
        return "CPU Brand String not available";
    }

    memset(CPUBrandString, 0, sizeof(CPUBrandString));

    // Safely copy CPU brand string with bounds checking
    for (unsigned int i = 0x80000002; i <= std::min(nExIds, 0x80000004u); ++i)
    {
        __cpuid(i, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);

        size_t offset = (i - 0x80000002) * 16;
        if (offset + 16 <= sizeof(CPUBrandString)) {
            memcpy(CPUBrandString + offset, CPUInfo, sizeof(CPUInfo));
        }
    }
    
    // Ensure null termination
    CPUBrandString[sizeof(CPUBrandString) - 1] = '\0';
    
    string str(CPUBrandString);
    return str;
}

// getOsName, this will get the OS of the current computer
const char *getOsName()
{
#ifdef __linux__
    return "linux";
#elif _WIN32
    return "Windows 32-bit";
#elif _WIN64
    return "Windows 64-bit";
#elif __APPLE__ || __MACH__
    return "Mac OSX";
#elif __FreeBSD__
    return "FreeBSD";
#elif __unix || __unix__
    return "Unix";
#else
    return "Other";
#endif
}

// getCurrentUser, get current logged user matching 'who' command
string getCurrentUser()
{
    char* username = getenv("USER");
    if (username != nullptr) {
        return string(username);
    }
    return "unknown";
}

// getHostname, get hostname matching 'hostname' command
string getHostname()
{
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return string(hostname);
    }
    return "unknown";
}

// getTotalProcesses, get total process count matching 'top' Tasks count
int getTotalProcesses()
{
    DIR* proc_dir = opendir("/proc");
    if (proc_dir == nullptr) {
        return 0;
    }
    
    int count = 0;
    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        // Check if directory name is a number (PID)
        if (entry->d_type == DT_DIR) {
            bool is_pid = true;
            for (int i = 0; entry->d_name[i] != '\0'; i++) {
                if (!isdigit(entry->d_name[i])) {
                    is_pid = false;
                    break;
                }
            }
            if (is_pid && entry->d_name[0] != '\0') {
                count++;
            }
        }
    }
    
    closedir(proc_dir);
    return count;
}

// getCPUModel, get CPU model from /proc/cpuinfo
string getCPUModel()
{
    std::ifstream file("/proc/cpuinfo");
    if (!file.is_open()) {
        return "CPU model not available";
    }
    
    string line;
    while (std::getline(file, line)) {
        if (line.find("model name") != string::npos) {
            size_t colon_pos = line.find(':');
            if (colon_pos != string::npos) {
                string model = line.substr(colon_pos + 1);
                // Remove leading whitespace
                size_t start = model.find_first_not_of(" \t");
                if (start != string::npos) {
                    return model.substr(start);
                }
            }
        }
    }
    
    file.close();
    return "CPU model not found";
}

// Read CPU statistics from /proc/stat
CPUStats readCPUStats()
{
    CPUStats stats = {0};
    std::ifstream file("/proc/stat");
    
    if (file.is_open()) {
        string line;
        if (std::getline(file, line)) {
            // Parse the first line: cpu  user nice system idle iowait irq softirq steal guest guest_nice
            if (line.substr(0, 4) == "cpu ") {
                std::istringstream iss(line);
                string cpu_label;
                iss >> cpu_label >> stats.user >> stats.nice >> stats.system >> stats.idle 
                    >> stats.iowait >> stats.irq >> stats.softirq >> stats.steal 
                    >> stats.guest >> stats.guestNice;
            }
        }
        file.close();
    }
    
    return stats;
}

// Calculate CPU percentage from two stat readings
float calculateCPUPercent(const CPUStats& current, const CPUStats& previous)
{
    long long int prevTotal = previous.getTotal();
    long long int prevIdle = previous.getIdle();
    
    long long int currTotal = current.getTotal();
    long long int currIdle = current.getIdle();
    
    long long int totalDiff = currTotal - prevTotal;
    long long int idleDiff = currIdle - prevIdle;
    
    if (totalDiff == 0) {
        return 0.0f;
    }
    
    return (1.0f - (float)idleDiff / (float)totalDiff) * 100.0f;
}

// Update CPU monitor with new data
void updateCPUMonitor(CPUMonitor& monitor)
{
    using std::chrono::steady_clock;
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    
    if (monitor.isPaused) {
        return;
    }
    
    auto now = steady_clock::now();
    auto elapsed = duration_cast<milliseconds>(now - monitor.lastUpdateTime).count();
    
    // Update based on configured update rate (convert FPS to milliseconds)
    float updateInterval = 1000.0f / monitor.updateRate;
    
    if (elapsed >= updateInterval) {
        CPUStats currentStats = readCPUStats();
        
        if (!monitor.isFirstRead) {
            monitor.currentCPUPercent = calculateCPUPercent(currentStats, monitor.previousStats);
            
            // Add to history
            monitor.cpuHistory.push_back(monitor.currentCPUPercent);
            
            // Limit history size
            if (monitor.cpuHistory.size() > monitor.maxHistorySize) {
                monitor.cpuHistory.pop_front();
            }
        } else {
            monitor.isFirstRead = false;
        }
        
        monitor.previousStats = currentStats;
        monitor.lastUpdateTime = now;
    }
}

// Render CPU graph using ImGui
void renderCPUGraph(CPUMonitor& monitor)
{
    if (ImGui::BeginTabBar("CPUTabs")) {
        if (ImGui::BeginTabItem("CPU")) {
            // Controls
            ImGui::Text("CPU Usage: %.1f%%", monitor.currentCPUPercent);
            ImGui::SameLine();
            
            // Play/Pause button
            if (ImGui::Button(monitor.isPaused ? "Resume" : "Pause")) {
                monitor.isPaused = !monitor.isPaused;
            }
            
            // FPS slider
            ImGui::SliderFloat("Update Rate (FPS)", &monitor.updateRate, 1.0f, 120.0f, "%.1f");
            
            // Y-scale slider
            ImGui::SliderFloat("Y-Scale", &monitor.yScale, 50.0f, 200.0f, "%.1f%%");
            
            // Graph
            if (!monitor.cpuHistory.empty()) {
                // Convert deque to vector for ImGui
                vector<float> plotData(monitor.cpuHistory.begin(), monitor.cpuHistory.end());
                
                ImGui::PlotLines("CPU Usage", plotData.data(), plotData.size(), 
                               0, nullptr, 0.0f, monitor.yScale, ImVec2(0, 200));
                
                // Current percentage overlay
                ImGui::Text("Current: %.1f%% | Avg: %.1f%% | Max: %.1f%%", 
                           monitor.currentCPUPercent,
                           // Calculate average
                           std::accumulate(plotData.begin(), plotData.end(), 0.0f) / plotData.size(),
                           // Find maximum
                           *std::max_element(plotData.begin(), plotData.end()));
            }
            
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

// Read ThinkPad thermal sensor (primary source)
float readThinkPadThermal()
{
    std::ifstream file("/proc/acpi/ibm/thermal");
    if (!file.is_open()) {
        return -999.0f; // Error indicator
    }
    
    string line;
    if (std::getline(file, line)) {
        // Parse: "temperatures:	41 -128 0 0 0 0 0 0"
        size_t pos = line.find("temperatures:");
        if (pos != string::npos) {
            std::istringstream iss(line.substr(pos + 13)); // Skip "temperatures:"
            float temp;
            if (iss >> temp && temp > -100) { // Valid temperature range
                return temp;
            }
        }
    }
    
    file.close();
    return -999.0f;
}

// Read thermal zone temperature (fallback)
float readThermalZone(int zone)
{
    string path = "/sys/class/thermal/thermal_zone" + std::to_string(zone) + "/temp";
    std::ifstream file(path);
    if (!file.is_open()) {
        return -999.0f;
    }
    
    int milliTemp;
    if (file >> milliTemp) {
        file.close();
        return milliTemp / 1000.0f; // Convert millidegrees to degrees
    }
    
    file.close();
    return -999.0f;
}

// Read hardware monitor temperature (fallback)
float readHwmonTemp(const string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return -999.0f;
    }
    
    int milliTemp;
    if (file >> milliTemp) {
        file.close();
        return milliTemp / 1000.0f; // Convert millidegrees to degrees
    }
    
    file.close();
    return -999.0f;
}

// Discover available thermal sensors
vector<ThermalSensor> discoverThermalSensors()
{
    vector<ThermalSensor> sensors;
    
    // Try ThinkPad thermal first (most accurate)
    float thinkpadTemp = readThinkPadThermal();
    if (thinkpadTemp > -900) {
        sensors.emplace_back("ThinkPad CPU", "/proc/acpi/ibm/thermal");
        sensors.back().temperature = thinkpadTemp;
        sensors.back().isValid = true;
    }
    
    // Try thermal zones
    for (int i = 0; i < 8; i++) {
        float temp = readThermalZone(i);
        if (temp > -900) {
            string name = "Thermal Zone " + std::to_string(i);
            string path = "/sys/class/thermal/thermal_zone" + std::to_string(i) + "/temp";
            sensors.emplace_back(name, path);
            sensors.back().temperature = temp;
            sensors.back().isValid = true;
        }
    }
    
    // Try hardware monitors
    vector<string> hwmonPaths = {
        "/sys/class/hwmon/hwmon1/temp1_input",
        "/sys/class/hwmon/hwmon3/temp1_input",
        "/sys/class/hwmon/hwmon6/temp1_input",
        "/sys/class/hwmon/hwmon7/temp1_input"
    };
    
    for (size_t i = 0; i < hwmonPaths.size(); i++) {
        float temp = readHwmonTemp(hwmonPaths[i]);
        if (temp > -900) {
            string name = "HW Monitor " + std::to_string(i + 1);
            sensors.emplace_back(name, hwmonPaths[i]);
            sensors.back().temperature = temp;
            sensors.back().isValid = true;
        }
    }
    
    return sensors;
}

// Initialize thermal monitor
void initThermalMonitor(ThermalMonitor& monitor)
{
    monitor.sensors = discoverThermalSensors();
    
    // Set preferred source (ThinkPad if available, else first valid sensor)
    for (const auto& sensor : monitor.sensors) {
        if (sensor.source == "/proc/acpi/ibm/thermal") {
            monitor.preferredSource = sensor.source;
            break;
        }
    }
    
    if (monitor.preferredSource.empty() && !monitor.sensors.empty()) {
        monitor.preferredSource = monitor.sensors[0].source;
    }
}

// Update thermal monitor with new data
void updateThermalMonitor(ThermalMonitor& monitor)
{
    using std::chrono::steady_clock;
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    
    if (monitor.isPaused) {
        return;
    }
    
    auto now = steady_clock::now();
    auto elapsed = duration_cast<milliseconds>(now - monitor.lastUpdateTime).count();
    
    // Update based on configured update rate
    float updateInterval = 1000.0f / monitor.updateRate;
    
    if (elapsed >= updateInterval) {
        // Update all sensors
        float maxTemp = 0.0f;
        bool hasValidTemp = false;
        
        for (auto& sensor : monitor.sensors) {
            float temp = -999.0f;
            
            if (sensor.source == "/proc/acpi/ibm/thermal") {
                temp = readThinkPadThermal();
            } else if (sensor.source.find("thermal_zone") != string::npos) {
                // Extract zone number from path
                size_t pos = sensor.source.find("thermal_zone") + 12;
                size_t end = sensor.source.find("/", pos);
                if (end != string::npos) {
                    int zone = std::stoi(sensor.source.substr(pos, end - pos));
                    temp = readThermalZone(zone);
                }
            } else {
                temp = readHwmonTemp(sensor.source);
            }
            
            if (temp > -900) {
                sensor.temperature = temp;
                sensor.isValid = true;
                maxTemp = std::max(maxTemp, temp);
                hasValidTemp = true;
            } else {
                sensor.isValid = false;
            }
        }
        
        if (hasValidTemp) {
            monitor.currentMaxTemp = maxTemp;
            monitor.tempHistory.push_back(maxTemp);
            
            // Limit history size
            if (monitor.tempHistory.size() > monitor.maxHistorySize) {
                monitor.tempHistory.pop_front();
            }
        }
        
        monitor.lastUpdateTime = now;
    }
}

// Render thermal graph using ImGui
void renderThermalGraph(ThermalMonitor& monitor)
{
    ImGui::Text("Temperature Monitoring");
    ImGui::Separator();
    
    // Controls
    ImGui::Text("Max Temperature: %.1f°C", monitor.currentMaxTemp);
    ImGui::SameLine();
    
    // Play/Pause button
    if (ImGui::Button(monitor.isPaused ? "Resume" : "Pause")) {
        monitor.isPaused = !monitor.isPaused;
    }
    
    // FPS slider
    ImGui::SliderFloat("Update Rate (FPS)", &monitor.updateRate, 1.0f, 60.0f, "%.1f");
    
    // Y-scale slider
    ImGui::SliderFloat("Y-Scale (°C)", &monitor.yScale, 50.0f, 120.0f, "%.1f");
    
    // Sensor status
    ImGui::Text("Available Sensors:");
    for (const auto& sensor : monitor.sensors) {
        if (sensor.isValid) {
            ImGui::Text("  %s: %.1f°C", sensor.name.c_str(), sensor.temperature);
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "  %s: Not Available", sensor.name.c_str());
        }
    }
    
    // Graph
    if (!monitor.tempHistory.empty()) {
        // Convert deque to vector for ImGui
        vector<float> plotData(monitor.tempHistory.begin(), monitor.tempHistory.end());
        
        ImGui::PlotLines("Temperature", plotData.data(), plotData.size(), 
                       0, nullptr, 0.0f, monitor.yScale, ImVec2(0, 200));
        
        // Temperature statistics overlay
        float avgTemp = std::accumulate(plotData.begin(), plotData.end(), 0.0f) / plotData.size();
        float maxTemp = *std::max_element(plotData.begin(), plotData.end());
        float minTemp = *std::min_element(plotData.begin(), plotData.end());
        
        ImGui::Text("Current: %.1f°C | Avg: %.1f°C | Max: %.1f°C | Min: %.1f°C", 
                   monitor.currentMaxTemp, avgTemp, maxTemp, minTemp);
    } else {
        ImGui::Text("Collecting temperature data...");
    }
}

// Read memory information from /proc/meminfo
MemoryInfo readMemoryInfo()
{
    MemoryInfo info;
    std::ifstream file("/proc/meminfo");
    
    if (!file.is_open()) {
        return info;
    }
    
    string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        string key;
        long long value;
        string unit;
        
        if (iss >> key >> value >> unit) {
            if (key == "MemTotal:") {
                info.memTotal = value;
            } else if (key == "MemFree:") {
                info.memFree = value;
            } else if (key == "MemAvailable:") {
                info.memAvailable = value;
            } else if (key == "Buffers:") {
                info.buffers = value;
            } else if (key == "Cached:") {
                info.cached = value;
            } else if (key == "SwapTotal:") {
                info.swapTotal = value;
            } else if (key == "SwapFree:") {
                info.swapFree = value;
            }
        }
    }
    
    file.close();
    
    // Calculate derived values (matching free command)
    info.swapUsed = info.swapTotal - info.swapFree;
    info.memUsed = info.memTotal - info.memFree - info.buffers - info.cached;
    
    if (info.memTotal > 0) {
        info.memUsedPercent = (float)info.memUsed / info.memTotal * 100.0f;
    }
    
    if (info.swapTotal > 0) {
        info.swapUsedPercent = (float)info.swapUsed / info.swapTotal * 100.0f;
    }
    
    return info;
}

// Read disk information using statvfs
DiskInfo readDiskInfo(const string& mountpoint)
{
    DiskInfo info;
    info.mountpoint = mountpoint;
    
    struct statvfs stat;
    if (statvfs(mountpoint.c_str(), &stat) == 0) {
        info.total = (stat.f_blocks * stat.f_frsize) / 1024; // Convert to kB
        info.available = (stat.f_bavail * stat.f_frsize) / 1024; // Convert to kB
        info.used = info.total - info.available;
        
        if (info.total > 0) {
            info.usedPercent = (float)info.used / info.total * 100.0f;
        }
        
        // Try to get filesystem name from /proc/mounts
        std::ifstream mounts("/proc/mounts");
        if (mounts.is_open()) {
            string line;
            while (std::getline(mounts, line)) {
                std::istringstream iss(line);
                string device, mount, fstype;
                if (iss >> device >> mount >> fstype) {
                    if (mount == mountpoint) {
                        info.filesystem = device;
                        break;
                    }
                }
            }
            mounts.close();
        }
    }
    
    return info;
}

// Read process information from /proc/[pid]/stat
ProcessInfo readProcessInfo(int pid)
{
    ProcessInfo info;
    info.pid = pid;
    
    string statPath = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream file(statPath);
    
    if (!file.is_open()) {
        return info;
    }
    
    // Parse stat file - format: pid (comm) state ppid ...
    string line;
    if (std::getline(file, line)) {
        // Find the last ')' to handle process names with spaces/parentheses
        size_t lastParen = line.rfind(')');
        if (lastParen != string::npos) {
            // Extract process name between first '(' and last ')'
            size_t firstParen = line.find('(');
            if (firstParen != string::npos && firstParen < lastParen) {
                info.name = line.substr(firstParen + 1, lastParen - firstParen - 1);
            }
            
            // Parse the rest after the last ')'
            std::istringstream iss(line.substr(lastParen + 1));
            string state_str;
            int ppid, pgrp, session, tty_nr, tpgid;
            unsigned long flags, minflt, cminflt, majflt, cmajflt;
            unsigned long utime, stime;
            long cutime, cstime, priority, nice, num_threads, itrealvalue;
            unsigned long long starttime;
            unsigned long vsize;
            long rss;
            
            if (iss >> state_str >> ppid >> pgrp >> session >> tty_nr >> tpgid
                    >> flags >> minflt >> cminflt >> majflt >> cmajflt
                    >> utime >> stime >> cutime >> cstime >> priority >> nice
                    >> num_threads >> itrealvalue >> starttime >> vsize >> rss) {
                
                info.state = state_str[0];
                info.utime = utime;
                info.stime = stime;
                info.vsize = vsize / 1024; // Convert to kB
                info.rss = rss * 4; // RSS is in pages, assume 4kB pages
            }
        }
    }
    
    file.close();
    return info;
}

// Read all processes
vector<ProcessInfo> readProcessList()
{
    vector<ProcessInfo> processes;
    
    DIR* proc_dir = opendir("/proc");
    if (proc_dir == nullptr) {
        return processes;
    }
    
    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        // Check if directory name is a number (PID)
        if (entry->d_type == DT_DIR) {
            bool is_pid = true;
            for (int i = 0; entry->d_name[i] != '\0'; i++) {
                if (!isdigit(entry->d_name[i])) {
                    is_pid = false;
                    break;
                }
            }
            
            if (is_pid && entry->d_name[0] != '\0') {
                int pid = std::atoi(entry->d_name);
                ProcessInfo proc = readProcessInfo(pid);
                if (proc.pid > 0 && !proc.name.empty()) {
                    processes.push_back(proc);
                }
            }
        }
    }
    
    closedir(proc_dir);
    return processes;
}

// Calculate process CPU percentage
float calculateProcessCPU(const ProcessInfo& current, const ProcessInfo& previous, float deltaTime)
{
    if (deltaTime <= 0) {
        return 0.0f;
    }
    
    long long totalTime = (current.utime + current.stime) - (previous.utime + previous.stime);
    
    // Convert jiffies to seconds (assuming 100 Hz)
    float cpuTime = totalTime / 100.0f;
    
    return (cpuTime / deltaTime) * 100.0f;
}

// Format bytes to human readable format (matching free -h)
string formatBytes(long long bytes, bool binary)
{
    const char* units[] = {"B", "Ki", "Mi", "Gi", "Ti"};
    const char* units_dec[] = {"B", "K", "M", "G", "T"};
    
    double size = bytes;
    int unit = 0;
    double divisor = binary ? 1024.0 : 1000.0;
    
    while (size >= divisor && unit < 4) {
        size /= divisor;
        unit++;
    }
    
    char buffer[32];
    if (unit == 0) {
        snprintf(buffer, sizeof(buffer), "%lld%s", bytes, binary ? units[unit] : units_dec[unit]);
    } else {
        snprintf(buffer, sizeof(buffer), "%.1f%s", size, binary ? units[unit] : units_dec[unit]);
    }
    
    return string(buffer);
}

// Filter processes based on search text
void filterProcesses(MemoryProcessMonitor& monitor)
{
    // Store current selection state by PID
    std::set<int> selectedPIDs;
    for (const auto& proc : monitor.filteredProcesses) {
        if (proc.selected) {
            selectedPIDs.insert(proc.pid);
        }
    }
    
    monitor.filteredProcesses.clear();
    
    if (monitor.searchFilter.empty()) {
        monitor.filteredProcesses = monitor.processes;
    } else {
        string lowerFilter = monitor.searchFilter;
        std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);
        
        for (const auto& proc : monitor.processes) {
            string lowerName = proc.name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            
            if (lowerName.find(lowerFilter) != string::npos || 
                std::to_string(proc.pid).find(monitor.searchFilter) != string::npos) {
                monitor.filteredProcesses.push_back(proc);
            }
        }
    }
    
    // Restore selection state
    for (auto& proc : monitor.filteredProcesses) {
        proc.selected = selectedPIDs.count(proc.pid) > 0;
    }
}

// Sort processes by CPU or Memory
void sortProcesses(MemoryProcessMonitor& monitor)
{
    if (monitor.sortByCPU) {
        std::sort(monitor.filteredProcesses.begin(), monitor.filteredProcesses.end(),
                  [&](const ProcessInfo& a, const ProcessInfo& b) {
                      return monitor.sortAscending ? a.cpuPercent < b.cpuPercent : a.cpuPercent > b.cpuPercent;
                  });
    } else if (monitor.sortByMemory) {
        std::sort(monitor.filteredProcesses.begin(), monitor.filteredProcesses.end(),
                  [&](const ProcessInfo& a, const ProcessInfo& b) {
                      return monitor.sortAscending ? a.memPercent < b.memPercent : a.memPercent > b.memPercent;
                  });
    } else {
        // Sort by PID by default
        std::sort(monitor.filteredProcesses.begin(), monitor.filteredProcesses.end(),
                  [&](const ProcessInfo& a, const ProcessInfo& b) {
                      return monitor.sortAscending ? a.pid < b.pid : a.pid > b.pid;
                  });
    }
}

// Update memory and process monitor
void updateMemoryProcessMonitor(MemoryProcessMonitor& monitor)
{
    using std::chrono::steady_clock;
    using std::chrono::duration_cast;
    using std::chrono::seconds;
    
    auto now = steady_clock::now();
    auto elapsed = duration_cast<seconds>(now - monitor.lastUpdateTime).count();
    
    // Update every 2 seconds
    if (elapsed >= 2) {
        // Read memory and disk info
        monitor.memory = readMemoryInfo();
        monitor.disk = readDiskInfo("/");
        monitor.totalSystemMemory = monitor.memory.memTotal;
        
        // Read processes
        vector<ProcessInfo> newProcesses = readProcessList();
        
        // Calculate CPU percentages using previous readings
        for (auto& proc : newProcesses) {
            auto prevIt = monitor.previousProcesses.find(proc.pid);
            if (prevIt != monitor.previousProcesses.end() && elapsed > 0) {
                proc.cpuPercent = calculateProcessCPU(proc, prevIt->second, elapsed);
            }
            
            // Calculate memory percentage
            if (monitor.totalSystemMemory > 0) {
                proc.memPercent = (float)proc.rss / monitor.totalSystemMemory * 100.0f;
            }
        }
        
        // Update previous processes for next iteration
        monitor.previousProcesses.clear();
        for (const auto& proc : newProcesses) {
            monitor.previousProcesses[proc.pid] = proc;
        }
        
        monitor.processes = newProcesses;
        filterProcesses(monitor);
        sortProcesses(monitor);
        
        monitor.lastUpdateTime = now;
    }
}

// Render memory and process interface
void renderMemoryProcessInterface(MemoryProcessMonitor& monitor)
{
    // Memory Information Section
    ImGui::Text("Memory Information");
    ImGui::Separator();
    
    // RAM Usage
    string ramTotal = formatBytes(monitor.memory.memTotal * 1024);
    string ramUsed = formatBytes(monitor.memory.memUsed * 1024);
    string ramFree = formatBytes(monitor.memory.memFree * 1024);
    string ramAvailable = formatBytes(monitor.memory.memAvailable * 1024);
    string ramBuffCache = formatBytes((monitor.memory.buffers + monitor.memory.cached) * 1024);
    
    ImGui::Text("RAM:  %s total, %s used, %s free, %s buff/cache, %s available", 
               ramTotal.c_str(), ramUsed.c_str(), ramFree.c_str(), 
               ramBuffCache.c_str(), ramAvailable.c_str());
    
    // RAM Progress Bar
    float memProgress = monitor.memory.memUsedPercent / 100.0f;
    ImGui::ProgressBar(memProgress, ImVec2(0.0f, 0.0f), 
                      (std::to_string((int)monitor.memory.memUsedPercent) + "%").c_str());
    
    // SWAP Usage
    if (monitor.memory.swapTotal > 0) {
        string swapTotal = formatBytes(monitor.memory.swapTotal * 1024);
        string swapUsed = formatBytes(monitor.memory.swapUsed * 1024);
        string swapFree = formatBytes(monitor.memory.swapFree * 1024);
        
        ImGui::Text("Swap: %s total, %s used, %s free", 
                   swapTotal.c_str(), swapUsed.c_str(), swapFree.c_str());
        
        float swapProgress = monitor.memory.swapUsedPercent / 100.0f;
        ImGui::ProgressBar(swapProgress, ImVec2(0.0f, 0.0f), 
                          (std::to_string((int)monitor.memory.swapUsedPercent) + "%").c_str());
    }
    
    // Disk Usage
    ImGui::Spacing();
    ImGui::Text("Disk Usage (/)");
    string diskTotal = formatBytes(monitor.disk.total * 1024);
    string diskUsed = formatBytes(monitor.disk.used * 1024);
    string diskAvail = formatBytes(monitor.disk.available * 1024);
    
    ImGui::Text("Disk: %s (%s) - %s used, %s available", 
               diskTotal.c_str(), monitor.disk.filesystem.c_str(),
               diskUsed.c_str(), diskAvail.c_str());
    
    float diskProgress = monitor.disk.usedPercent / 100.0f;
    ImGui::ProgressBar(diskProgress, ImVec2(0.0f, 0.0f), 
                      (std::to_string((int)monitor.disk.usedPercent) + "%").c_str());
    
    // Process Table Section
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Text("Process List (%d processes)", (int)monitor.filteredProcesses.size());
    ImGui::Separator();
    
    // Search Filter
    char searchBuffer[256];
    strncpy(searchBuffer, monitor.searchFilter.c_str(), sizeof(searchBuffer) - 1);
    searchBuffer[sizeof(searchBuffer) - 1] = '\0';
    
    if (ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer))) {
        monitor.searchFilter = searchBuffer;
        filterProcesses(monitor);
        sortProcesses(monitor);
    }
    
    ImGui::SameLine();
    
    // Sort buttons
    if (ImGui::Button("Sort by CPU")) {
        monitor.sortByCPU = true;
        monitor.sortByMemory = false;
        monitor.sortAscending = !monitor.sortAscending;
        sortProcesses(monitor);
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Sort by Memory")) {
        monitor.sortByCPU = false;
        monitor.sortByMemory = true;
        monitor.sortAscending = !monitor.sortAscending;
        sortProcesses(monitor);
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Sort by PID")) {
        monitor.sortByCPU = false;
        monitor.sortByMemory = false;
        monitor.sortAscending = !monitor.sortAscending;
        sortProcesses(monitor);
    }
    
    // Process Table
    if (ImGui::BeginTable("ProcessTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | 
                         ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 300))) {
        
        ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 50);
        ImGui::TableSetupColumn("CPU%", ImGuiTableColumnFlags_WidthFixed, 70);
        ImGui::TableSetupColumn("Memory%", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("RSS", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableHeadersRow();
        
        // Limit displayed processes for performance
        int maxDisplayed = std::min(100, (int)monitor.filteredProcesses.size());
        
        for (int i = 0; i < maxDisplayed; i++) {
            auto& proc = monitor.filteredProcesses[i];
            
            ImGui::TableNextRow();
            
            // PID column with selection
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable(std::to_string(proc.pid).c_str(), proc.selected, 
                                 ImGuiSelectableFlags_SpanAllColumns)) {
                // Toggle selection (multi-select with Ctrl)
                if (ImGui::GetIO().KeyCtrl) {
                    proc.selected = !proc.selected;
                } else {
                    // Single select - clear others
                    for (auto& p : monitor.filteredProcesses) {
                        p.selected = false;
                    }
                    proc.selected = true;
                }
            }
            
            // Name column
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", proc.name.c_str());
            
            // State column
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%c", proc.state);
            
            // CPU% column
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.1f", proc.cpuPercent);
            
            // Memory% column
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%.1f", proc.memPercent);
            
            // RSS column
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%s", formatBytes(proc.rss * 1024).c_str());
        }
        
        ImGui::EndTable();
    }
    
    // Selection info
    int selectedCount = 0;
    for (const auto& proc : monitor.filteredProcesses) {
        if (proc.selected) selectedCount++;
    }
    
    if (selectedCount > 0) {
        ImGui::Text("Selected: %d process%s", selectedCount, selectedCount == 1 ? "" : "es");
    }
}

// Read network interface statistics from /proc/net/dev
vector<NetworkInterfaceStats> readNetworkInterfaces()
{
    vector<NetworkInterfaceStats> interfaces;
    std::ifstream file("/proc/net/dev");
    
    if (!file.is_open()) {
        return interfaces;
    }
    
    string line;
    // Skip header lines
    std::getline(file, line); // Inter-|   Receive   ...
    std::getline(file, line); //  face |bytes    packets ...
    
    while (std::getline(file, line)) {
        NetworkInterfaceStats iface;
        
        // Parse interface name (before colon)
        size_t colonPos = line.find(':');
        if (colonPos == string::npos) continue;
        
        string ifaceName = line.substr(0, colonPos);
        // Trim whitespace
        ifaceName.erase(0, ifaceName.find_first_not_of(" \t"));
        ifaceName.erase(ifaceName.find_last_not_of(" \t") + 1);
        iface.name = ifaceName;
        
        // Parse statistics (after colon)
        string stats = line.substr(colonPos + 1);
        std::istringstream iss(stats);
        
        // RX: bytes packets errs drop fifo frame compressed multicast
        // TX: bytes packets errs drop fifo colls carrier compressed
        if (iss >> iface.rxBytes >> iface.rxPackets >> iface.rxErrs >> iface.rxDrop 
                >> iface.rxFifo >> iface.rxFrame >> iface.rxCompressed >> iface.rxMulticast
                >> iface.txBytes >> iface.txPackets >> iface.txErrs >> iface.txDrop
                >> iface.txFifo >> iface.txColls >> iface.txCarrier >> iface.txCompressed) {
            interfaces.push_back(iface);
        }
    }
    
    file.close();
    return interfaces;
}

// Get IP addresses for interfaces
map<string, string> getInterfaceIPAddresses()
{
    map<string, string> ipAddresses;
    
    struct ifaddrs *ifaddrsPtr;
    if (getifaddrs(&ifaddrsPtr) == -1) {
        return ipAddresses;
    }
    
    for (struct ifaddrs *ifa = ifaddrsPtr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *addr_in = (struct sockaddr_in *)ifa->ifa_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(addr_in->sin_addr), addressBuffer, INET_ADDRSTRLEN);
            ipAddresses[ifa->ifa_name] = string(addressBuffer);
        }
    }
    
    freeifaddrs(ifaddrsPtr);
    return ipAddresses;
}

// Get interface states from ip command or /sys/class/net
map<string, string> getInterfaceStates()
{
    map<string, string> states;
    
    // Try to read from /sys/class/net/[interface]/operstate
    DIR* netDir = opendir("/sys/class/net");
    if (netDir) {
        struct dirent* entry;
        while ((entry = readdir(netDir)) != nullptr) {
            if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
                string ifaceName = entry->d_name;
                string statePath = "/sys/class/net/" + ifaceName + "/operstate";
                
                std::ifstream stateFile(statePath);
                if (stateFile.is_open()) {
                    string state;
                    if (std::getline(stateFile, state)) {
                        // Convert to uppercase for consistency
                        std::transform(state.begin(), state.end(), state.begin(), ::toupper);
                        states[ifaceName] = state;
                    }
                    stateFile.close();
                }
            }
        }
        closedir(netDir);
    }
    
    return states;
}

// Format network speed for display
string formatNetworkSpeed(float bytesPerSecond)
{
    const char* units[] = {"B/s", "KB/s", "MB/s", "GB/s"};
    double speed = bytesPerSecond;
    int unit = 0;
    
    while (speed >= 1024.0 && unit < 3) {
        speed /= 1024.0;
        unit++;
    }
    
    char buffer[32];
    if (speed < 10.0) {
        snprintf(buffer, sizeof(buffer), "%.2f %s", speed, units[unit]);
    } else if (speed < 100.0) {
        snprintf(buffer, sizeof(buffer), "%.1f %s", speed, units[unit]);
    } else {
        snprintf(buffer, sizeof(buffer), "%.0f %s", speed, units[unit]);
    }
    
    return string(buffer);
}

// Format network bytes (similar to formatBytes but optimized for network)
string formatNetworkBytes(long long bytes)
{
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double size = bytes;
    int unit = 0;
    
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    
    char buffer[32];
    if (unit == 0) {
        snprintf(buffer, sizeof(buffer), "%lld %s", bytes, units[unit]);
    } else if (size < 10.0) {
        snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unit]);
    } else if (size < 100.0) {
        snprintf(buffer, sizeof(buffer), "%.1f %s", size, units[unit]);
    } else {
        snprintf(buffer, sizeof(buffer), "%.0f %s", size, units[unit]);
    }
    
    return string(buffer);
}

// Update network monitor with new data
void updateNetworkMonitor(NetworkMonitor& monitor)
{
    using std::chrono::steady_clock;
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    
    if (monitor.isPaused) {
        return;
    }
    
    auto now = steady_clock::now();
    auto elapsed = duration_cast<milliseconds>(now - monitor.lastUpdateTime).count();
    
    // Update based on configured update rate
    float updateInterval = 1000.0f / monitor.updateRate;
    
    if (elapsed >= updateInterval) {
        // Read current interface stats
        vector<NetworkInterfaceStats> newInterfaces = readNetworkInterfaces();
        map<string, string> ipAddresses = getInterfaceIPAddresses();
        map<string, string> states = getInterfaceStates();
        
        // Calculate speeds using previous readings
        float totalRxSpeed = 0.0f;
        float totalTxSpeed = 0.0f;
        
        for (auto& iface : newInterfaces) {
            // Add IP address and state
            if (ipAddresses.find(iface.name) != ipAddresses.end()) {
                iface.ipAddress = ipAddresses[iface.name];
            }
            if (states.find(iface.name) != states.end()) {
                iface.state = states[iface.name];
            }
            
            // Calculate speed if we have previous data
            auto prevIt = monitor.previousStats.find(iface.name);
            if (prevIt != monitor.previousStats.end() && elapsed > 0) {
                float deltaTime = elapsed / 1000.0f; // Convert to seconds
                
                long long rxDelta = iface.rxBytes - prevIt->second.rxBytes;
                long long txDelta = iface.txBytes - prevIt->second.txBytes;
                
                iface.rxSpeed = rxDelta / deltaTime;
                iface.txSpeed = txDelta / deltaTime;
                
                // Skip loopback for total speed calculation
                if (iface.name != "lo") {
                    totalRxSpeed += iface.rxSpeed;
                    totalTxSpeed += iface.txSpeed;
                }
            }
        }
        
        // Update history with total speeds
        monitor.rxSpeedHistory.push_back(totalRxSpeed);
        monitor.txSpeedHistory.push_back(totalTxSpeed);
        
        // Limit history size
        if (monitor.rxSpeedHistory.size() > monitor.maxHistorySize) {
            monitor.rxSpeedHistory.pop_front();
        }
        if (monitor.txSpeedHistory.size() > monitor.maxHistorySize) {
            monitor.txSpeedHistory.pop_front();
        }
        
        // Update max speeds for scaling
        monitor.maxRxSpeed = std::max(monitor.maxRxSpeed, totalRxSpeed);
        monitor.maxTxSpeed = std::max(monitor.maxTxSpeed, totalTxSpeed);
        
        // Store current stats for next iteration
        monitor.previousStats.clear();
        for (const auto& iface : newInterfaces) {
            monitor.previousStats[iface.name] = iface;
        }
        
        monitor.interfaces = newInterfaces;
        monitor.lastUpdateTime = now;
    }
}

// Render network interface with tabbed visualization
void renderNetworkInterface(NetworkMonitor& monitor)
{
    ImGui::Text("Network Monitoring");
    ImGui::Separator();
    
    // Control buttons
    if (ImGui::Button(monitor.isPaused ? "Resume" : "Pause")) {
        monitor.isPaused = !monitor.isPaused;
    }
    
    ImGui::SameLine();
    ImGui::SliderFloat("Update Rate", &monitor.updateRate, 0.5f, 10.0f, "%.1f Hz");
    
    // Interface selection
    if (!monitor.interfaces.empty()) {
        ImGui::Text("Select Interface:");
        ImGui::SameLine();
        
        if (ImGui::BeginCombo("##InterfaceSelect", monitor.selectedInterface.empty() ? "All Interfaces" : monitor.selectedInterface.c_str())) {
            if (ImGui::Selectable("All Interfaces", monitor.selectedInterface.empty())) {
                monitor.selectedInterface = "";
            }
            
            for (const auto& iface : monitor.interfaces) {
                bool isSelected = (monitor.selectedInterface == iface.name);
                if (ImGui::Selectable(iface.name.c_str(), isSelected)) {
                    monitor.selectedInterface = iface.name;
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }
    
    // Create tabbed interface for RX and TX
    if (ImGui::BeginTabBar("NetworkTabs")) {
        // Interface Overview Tab
        if (ImGui::BeginTabItem("Overview")) {
            ImGui::Text("Network Interfaces");
            ImGui::Separator();
            
            // Interface table
            if (ImGui::BeginTable("NetworkTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | 
                                 ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 250))) {
                
                ImGui::TableSetupColumn("Interface", ImGuiTableColumnFlags_WidthFixed, 120);
                ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 70);
                ImGui::TableSetupColumn("IP Address", ImGuiTableColumnFlags_WidthFixed, 120);
                ImGui::TableSetupColumn("RX Speed", ImGuiTableColumnFlags_WidthFixed, 100);
                ImGui::TableSetupColumn("TX Speed", ImGuiTableColumnFlags_WidthFixed, 100);
                ImGui::TableSetupColumn("RX Total", ImGuiTableColumnFlags_WidthFixed, 100);
                ImGui::TableSetupColumn("TX Total", ImGuiTableColumnFlags_WidthFixed, 100);
                ImGui::TableHeadersRow();
                
                for (const auto& iface : monitor.interfaces) {
                    ImGui::TableNextRow();
                    
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", iface.name.c_str());
                    
                    ImGui::TableSetColumnIndex(1);
                    if (iface.state == "UP") {
                        ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", iface.state.c_str());
                    } else {
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1), "%s", iface.state.c_str());
                    }
                    
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%s", iface.ipAddress.empty() ? "-" : iface.ipAddress.c_str());
                    
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%s", formatNetworkSpeed(iface.rxSpeed).c_str());
                    
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%s", formatNetworkSpeed(iface.txSpeed).c_str());
                    
                    ImGui::TableSetColumnIndex(5);
                    ImGui::Text("%s", formatNetworkBytes(iface.rxBytes).c_str());
                    
                    ImGui::TableSetColumnIndex(6);
                    ImGui::Text("%s", formatNetworkBytes(iface.txBytes).c_str());
                }
                
                ImGui::EndTable();
            }
            
            ImGui::EndTabItem();
        }
        
        // RX Visualization Tab
        if (ImGui::BeginTabItem("RX (Download)")) {
            if (!monitor.rxSpeedHistory.empty()) {
                // Current speed display
                float currentRxSpeed = monitor.rxSpeedHistory.back();
                ImGui::Text("Current RX Speed: %s", formatNetworkSpeed(currentRxSpeed).c_str());
                
                // Calculate scale for graph
                float maxScale = std::max(monitor.maxRxSpeed * 1.1f, 1024.0f); // At least 1KB/s
                
                // Convert deque to vector for ImGui
                vector<float> plotData(monitor.rxSpeedHistory.begin(), monitor.rxSpeedHistory.end());
                
                ImGui::PlotLines("RX Speed", plotData.data(), plotData.size(), 
                               0, nullptr, 0.0f, maxScale, ImVec2(0, 200));
                
                // Statistics
                if (plotData.size() > 0) {
                    float avgSpeed = std::accumulate(plotData.begin(), plotData.end(), 0.0f) / plotData.size();
                    float maxSpeed = *std::max_element(plotData.begin(), plotData.end());
                    
                    ImGui::Text("Average: %s | Peak: %s", 
                               formatNetworkSpeed(avgSpeed).c_str(),
                               formatNetworkSpeed(maxSpeed).c_str());
                }
            } else {
                ImGui::Text("Collecting RX data...");
            }
            
            ImGui::EndTabItem();
        }
        
        // TX Visualization Tab
        if (ImGui::BeginTabItem("TX (Upload)")) {
            if (!monitor.txSpeedHistory.empty()) {
                // Current speed display
                float currentTxSpeed = monitor.txSpeedHistory.back();
                ImGui::Text("Current TX Speed: %s", formatNetworkSpeed(currentTxSpeed).c_str());
                
                // Calculate scale for graph
                float maxScale = std::max(monitor.maxTxSpeed * 1.1f, 1024.0f); // At least 1KB/s
                
                // Convert deque to vector for ImGui
                vector<float> plotData(monitor.txSpeedHistory.begin(), monitor.txSpeedHistory.end());
                
                ImGui::PlotLines("TX Speed", plotData.data(), plotData.size(), 
                               0, nullptr, 0.0f, maxScale, ImVec2(0, 200));
                
                // Statistics
                if (plotData.size() > 0) {
                    float avgSpeed = std::accumulate(plotData.begin(), plotData.end(), 0.0f) / plotData.size();
                    float maxSpeed = *std::max_element(plotData.begin(), plotData.end());
                    
                    ImGui::Text("Average: %s | Peak: %s", 
                               formatNetworkSpeed(avgSpeed).c_str(),
                               formatNetworkSpeed(maxSpeed).c_str());
                }
            } else {
                ImGui::Text("Collecting TX data...");
            }
            
            ImGui::EndTabItem();
        }
        
        // Combined RX/TX Tab
        if (ImGui::BeginTabItem("Combined")) {
            if (!monitor.rxSpeedHistory.empty() && !monitor.txSpeedHistory.empty()) {
                ImGui::Text("Network Activity");
                
                float maxScale = std::max({monitor.maxRxSpeed, monitor.maxTxSpeed, 1024.0f}) * 1.1f;
                
                // RX Graph
                vector<float> rxData(monitor.rxSpeedHistory.begin(), monitor.rxSpeedHistory.end());
                ImGui::Text("Download Speed");
                ImGui::PlotLines("##RX", rxData.data(), rxData.size(), 
                               0, nullptr, 0.0f, maxScale, ImVec2(0, 100));
                
                // TX Graph
                vector<float> txData(monitor.txSpeedHistory.begin(), monitor.txSpeedHistory.end());
                ImGui::Text("Upload Speed");
                ImGui::PlotLines("##TX", txData.data(), txData.size(), 
                               0, nullptr, 0.0f, maxScale, ImVec2(0, 100));
                
                // Current speeds
                if (!rxData.empty() && !txData.empty()) {
                    ImGui::Text("Current: ↓ %s | ↑ %s", 
                               formatNetworkSpeed(rxData.back()).c_str(),
                               formatNetworkSpeed(txData.back()).c_str());
                }
            } else {
                ImGui::Text("Collecting network data...");
            }
            
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
}

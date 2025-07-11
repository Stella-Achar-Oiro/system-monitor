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
#include <set>
#include <algorithm>
#include <deque>
#include <chrono>
#include <sstream>
#include <numeric>
#include <sys/statvfs.h>
#include <cctype>
#include <fstream>
#include <ctime>
#include <thread>

// Avoid global namespace pollution - use specific declarations
using std::string;
using std::vector;
using std::map;
using std::deque;

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
    
    // Calculate total and idle time
    long long int getTotal() const {
        return user + nice + system + idle + iowait + irq + softirq + steal + guest + guestNice;
    }
    
    long long int getIdle() const {
        return idle + iowait;
    }
};

// CPU monitoring data structure
struct CPUMonitor
{
    deque<float> cpuHistory;
    CPUStats previousStats;
    float currentCPUPercent;
    bool isFirstRead;
    float maxHistorySize;
    bool isPaused;
    float updateRate;
    float yScale;
    std::chrono::steady_clock::time_point lastUpdateTime;
    
    CPUMonitor() : currentCPUPercent(0.0f), isFirstRead(true), 
                   maxHistorySize(200), isPaused(false), updateRate(60.0f), yScale(100.0f) {}
};

// Thermal monitoring data structure
struct ThermalSensor
{
    string name;
    float temperature;
    bool isValid;
    string source;  // Source file path
    
    ThermalSensor() : temperature(0.0f), isValid(false) {}
    ThermalSensor(const string& n, const string& src) : name(n), temperature(0.0f), isValid(false), source(src) {}
};

struct ThermalMonitor
{
    vector<ThermalSensor> sensors;
    deque<float> tempHistory;
    float currentMaxTemp;
    float maxHistorySize;
    bool isPaused;
    float updateRate;
    float yScale;
    std::chrono::steady_clock::time_point lastUpdateTime;
    string preferredSource;
    
    ThermalMonitor() : currentMaxTemp(0.0f), maxHistorySize(200), isPaused(false), 
                       updateRate(60.0f), yScale(100.0f), preferredSource("") {}
};

// Memory monitoring data structures
struct MemoryInfo
{
    long long memTotal;     // Total RAM in kB
    long long memFree;      // Free RAM in kB  
    long long memAvailable; // Available RAM in kB
    long long buffers;      // Buffer cache in kB
    long long cached;       // Page cache in kB
    long long swapTotal;    // Total swap in kB
    long long swapFree;     // Free swap in kB
    long long swapUsed;     // Used swap in kB
    
    // Calculated values
    long long memUsed;      // Used RAM in kB
    float memUsedPercent;   // Memory usage percentage
    float swapUsedPercent;  // Swap usage percentage
    
    MemoryInfo() : memTotal(0), memFree(0), memAvailable(0), buffers(0), cached(0),
                   swapTotal(0), swapFree(0), swapUsed(0), memUsed(0), 
                   memUsedPercent(0.0f), swapUsedPercent(0.0f) {}
};

struct DiskInfo
{
    string filesystem;
    string mountpoint;
    long long total;        // Total space in kB
    long long used;         // Used space in kB
    long long available;    // Available space in kB
    float usedPercent;      // Usage percentage
    
    DiskInfo() : total(0), used(0), available(0), usedPercent(0.0f) {}
};

struct ProcessInfo
{
    int pid;
    string name;
    char state;
    long long vsize;        // Virtual memory size in kB
    long long rss;          // Resident set size in kB
    long long utime;        // User time in jiffies
    long long stime;        // System time in jiffies
    float cpuPercent;       // CPU usage percentage
    float memPercent;       // Memory usage percentage
    bool selected;          // Selection state for multi-select
    
    ProcessInfo() : pid(0), state('?'), vsize(0), rss(0), utime(0), stime(0),
                    cpuPercent(0.0f), memPercent(0.0f), selected(false) {}
};

struct MemoryProcessMonitor
{
    MemoryInfo memory;
    DiskInfo disk;
    vector<ProcessInfo> processes;
    vector<ProcessInfo> filteredProcesses;
    string searchFilter;
    bool sortByCPU;
    bool sortByMemory;
    bool sortAscending;
    long long totalSystemMemory;
    map<int, ProcessInfo> previousProcesses; // For CPU calculation
    std::chrono::steady_clock::time_point lastUpdateTime;
    
    MemoryProcessMonitor() : sortByCPU(true), sortByMemory(false), sortAscending(false),
                            totalSystemMemory(0) {}
};

// Network monitoring data structures  
struct NetworkInterfaceStats
{
    string name;
    long long rxBytes, rxPackets, rxErrs, rxDrop, rxFifo, rxFrame, rxCompressed, rxMulticast;
    long long txBytes, txPackets, txErrs, txDrop, txFifo, txColls, txCarrier, txCompressed;
    
    // IP address information
    string ipAddress;
    string state;  // UP, DOWN, etc.
    
    // Speed calculations
    float rxSpeed;  // bytes per second
    float txSpeed;  // bytes per second
    
    NetworkInterfaceStats() : rxBytes(0), rxPackets(0), rxErrs(0), rxDrop(0), rxFifo(0), 
                             rxFrame(0), rxCompressed(0), rxMulticast(0),
                             txBytes(0), txPackets(0), txErrs(0), txDrop(0), txFifo(0),
                             txColls(0), txCarrier(0), txCompressed(0),
                             rxSpeed(0.0f), txSpeed(0.0f) {}
};

struct NetworkMonitor
{
    vector<NetworkInterfaceStats> interfaces;
    map<string, NetworkInterfaceStats> previousStats;
    deque<float> rxSpeedHistory;
    deque<float> txSpeedHistory;
    float maxRxSpeed;
    float maxTxSpeed;
    float maxHistorySize;
    bool isPaused;
    float updateRate;
    std::chrono::steady_clock::time_point lastUpdateTime;
    string selectedInterface;
    
    NetworkMonitor() : maxRxSpeed(0.0f), maxTxSpeed(0.0f), maxHistorySize(200), 
                       isPaused(false), updateRate(2.0f), selectedInterface("") {}
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
    string name;  // Use string instead of char* to avoid memory management issues
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

// Enhanced System Monitor Structures

// Historical data point with timestamp
struct HistoricalDataPoint
{
    std::chrono::steady_clock::time_point timestamp;
    float value;
    
    HistoricalDataPoint() : value(0.0f) {
        timestamp = std::chrono::steady_clock::now();
    }
    
    HistoricalDataPoint(float v) : value(v) {
        timestamp = std::chrono::steady_clock::now();
    }
};

// Performance alert configuration
struct AlertThreshold
{
    string name;
    float warningLevel;
    float criticalLevel;
    bool enabled;
    bool isActive;
    std::chrono::steady_clock::time_point lastTriggered;
    
    AlertThreshold() : warningLevel(80.0f), criticalLevel(95.0f), 
                       enabled(true), isActive(false) {}
    
    AlertThreshold(const string& n, float warning, float critical) 
        : name(n), warningLevel(warning), criticalLevel(critical), 
          enabled(true), isActive(false) {}
};

// System configuration for user preferences
struct SystemConfig
{
    // Display preferences
    float refreshRate;
    int maxHistoryPoints;
    bool enableAlerts;
    bool enableTrendAnalysis;
    bool enableDataExport;
    
    // Alert thresholds
    AlertThreshold cpuAlert;
    AlertThreshold memoryAlert;
    AlertThreshold diskAlert;
    AlertThreshold temperatureAlert;
    
    // Export settings
    string exportPath;
    string exportFormat; // "csv", "json", "xml"
    
    // Trend analysis settings
    int trendAnalysisPeriod; // minutes
    float trendSensitivity;
    
    SystemConfig() : refreshRate(60.0f), maxHistoryPoints(1000), 
                     enableAlerts(true), enableTrendAnalysis(true), 
                     enableDataExport(true), exportPath("./exports/"), 
                     exportFormat("csv"), trendAnalysisPeriod(60), 
                     trendSensitivity(1.0f) {
        cpuAlert = AlertThreshold("CPU Usage", 80.0f, 95.0f);
        memoryAlert = AlertThreshold("Memory Usage", 85.0f, 95.0f);
        diskAlert = AlertThreshold("Disk Usage", 90.0f, 98.0f);
        temperatureAlert = AlertThreshold("Temperature", 70.0f, 85.0f);
    }
};

// Trend analysis results
struct TrendAnalysis
{
    string metric;
    float currentValue;
    float averageValue;
    float trend; // Positive = increasing, Negative = decreasing
    float prediction; // Predicted value for next period
    string analysis; // Human-readable analysis
    bool isAnomalous; // Detected anomaly
    
    TrendAnalysis() : currentValue(0.0f), averageValue(0.0f), 
                      trend(0.0f), prediction(0.0f), isAnomalous(false) {}
};

// Alert notification
struct AlertNotification
{
    string title;
    string message;
    string severity; // "warning", "critical", "info"
    std::chrono::steady_clock::time_point timestamp;
    bool acknowledged;
    
    AlertNotification() : acknowledged(false) {
        timestamp = std::chrono::steady_clock::now();
    }
    
    AlertNotification(const string& t, const string& m, const string& s)
        : title(t), message(m), severity(s), acknowledged(false) {
        timestamp = std::chrono::steady_clock::now();
    }
};

// Enhanced historical data storage
struct HistoricalData
{
    deque<HistoricalDataPoint> cpuHistory;
    deque<HistoricalDataPoint> memoryHistory;
    deque<HistoricalDataPoint> diskHistory;
    deque<HistoricalDataPoint> temperatureHistory;
    deque<HistoricalDataPoint> networkRxHistory;
    deque<HistoricalDataPoint> networkTxHistory;
    
    // Trend analysis results
    vector<TrendAnalysis> trendResults;
    
    // Alert notifications
    deque<AlertNotification> notifications;
    
    // Configuration
    SystemConfig config;
    
    // Statistics
    std::chrono::steady_clock::time_point startTime;
    long long totalDataPoints;
    
    HistoricalData() : totalDataPoints(0) {
        startTime = std::chrono::steady_clock::now();
    }
    
    void addDataPoint(const string& metric, float value);
    void trimHistory();
    void saveToFile(const string& filename);
    void loadFromFile(const string& filename);
    void exportData(const string& format, const string& filename);
    TrendAnalysis calculateTrend(const string& metric);
    void checkAlerts();
    void addNotification(const AlertNotification& notification);
};

// student TODO : system stats
string CPUinfo();
const char *getOsName();
string getCurrentUser();
string getHostname();
int getTotalProcesses();
string getCPUModel();

// CPU monitoring functions
CPUStats readCPUStats();
float calculateCPUPercent(const CPUStats& current, const CPUStats& previous);
void updateCPUMonitor(CPUMonitor& monitor);
void renderCPUGraph(CPUMonitor& monitor);

// Thermal monitoring functions
void initThermalMonitor(ThermalMonitor& monitor);
vector<ThermalSensor> discoverThermalSensors();
float readThinkPadThermal();
float readThermalZone(int zone);
float readHwmonTemp(const string& path);
void updateThermalMonitor(ThermalMonitor& monitor);
void renderThermalGraph(ThermalMonitor& monitor);

// Memory and process monitoring functions
MemoryInfo readMemoryInfo();
DiskInfo readDiskInfo(const string& mountpoint);
vector<ProcessInfo> readProcessList();
ProcessInfo readProcessInfo(int pid);
float calculateProcessCPU(const ProcessInfo& current, const ProcessInfo& previous, float deltaTime);
void updateMemoryProcessMonitor(MemoryProcessMonitor& monitor);
void renderMemoryProcessInterface(MemoryProcessMonitor& monitor);
string formatBytes(long long bytes, bool binary = true);
void filterProcesses(MemoryProcessMonitor& monitor);
void sortProcesses(MemoryProcessMonitor& monitor);

// Network monitoring functions
vector<NetworkInterfaceStats> readNetworkInterfaces();
map<string, string> getInterfaceIPAddresses();
map<string, string> getInterfaceStates();
void updateNetworkMonitor(NetworkMonitor& monitor);
void renderNetworkInterface(NetworkMonitor& monitor);
string formatNetworkSpeed(float bytesPerSecond);
string formatNetworkBytes(long long bytes);

// Enhanced System Monitor Functions

// Historical data management
void initializeHistoricalData(HistoricalData& data);
void updateHistoricalData(HistoricalData& data, const CPUMonitor& cpu, 
                         const MemoryProcessMonitor& memory, const ThermalMonitor& thermal,
                         const NetworkMonitor& network);

// Configuration management  
bool loadConfiguration(SystemConfig& config, const string& filename = "monitor_config.json");
bool saveConfiguration(const SystemConfig& config, const string& filename = "monitor_config.json");
void renderConfigurationInterface(SystemConfig& config);

// Alert system
void updateAlerts(HistoricalData& data, const CPUMonitor& cpu, 
                 const MemoryProcessMonitor& memory, const ThermalMonitor& thermal);
void renderAlertsInterface(HistoricalData& data);
string formatAlertMessage(const AlertThreshold& alert, float currentValue);

// Trend analysis
TrendAnalysis performTrendAnalysis(const deque<HistoricalDataPoint>& history, 
                                  const string& metricName);
void updateTrendAnalysis(HistoricalData& data);
void renderTrendAnalysisInterface(const HistoricalData& data);

// Export functionality
bool exportToCSV(const HistoricalData& data, const string& filename);
bool exportToJSON(const HistoricalData& data, const string& filename);
void renderExportInterface(HistoricalData& data);

// Enhanced UI components
void renderHistoricalGraphs(const HistoricalData& data);
void renderAdvancedSystemOverview(const HistoricalData& data, const CPUMonitor& cpu,
                                const MemoryProcessMonitor& memory, const ThermalMonitor& thermal);
void renderNotificationsPanel(HistoricalData& data);

// Utility functions
string formatDuration(std::chrono::steady_clock::duration duration);
string formatTimestamp(std::chrono::steady_clock::time_point timestamp);
float calculateMovingAverage(const deque<HistoricalDataPoint>& history, int periods);
float calculateStandardDeviation(const deque<HistoricalDataPoint>& history);

// Global data accessor
HistoricalData& getHistoricalData();

// ===== UI POLISH AND DESIGN SYSTEM =====

// Color scheme definitions
struct UIColorScheme {
    // Primary colors
    ImVec4 primary;
    ImVec4 primaryLight;
    ImVec4 primaryDark;
    
    // Status colors
    ImVec4 success;
    ImVec4 warning;
    ImVec4 danger;
    ImVec4 info;
    
    // Graph colors
    ImVec4 cpuColor;
    ImVec4 memoryColor;
    ImVec4 diskColor;
    ImVec4 temperatureColor;
    ImVec4 networkRxColor;
    ImVec4 networkTxColor;
    
    // UI colors
    ImVec4 textPrimary;
    ImVec4 textSecondary;
    ImVec4 textMuted;
    ImVec4 background;
    ImVec4 backgroundSecondary;
    ImVec4 border;
    
    UIColorScheme() {
        // Modern dark theme with accent colors
        primary = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        primaryLight = ImVec4(0.40f, 0.70f, 1.00f, 1.00f);
        primaryDark = ImVec4(0.15f, 0.45f, 0.80f, 1.00f);
        
        success = ImVec4(0.20f, 0.80f, 0.20f, 1.00f);
        warning = ImVec4(1.00f, 0.65f, 0.00f, 1.00f);
        danger = ImVec4(0.90f, 0.20f, 0.20f, 1.00f);
        info = ImVec4(0.30f, 0.80f, 1.00f, 1.00f);
        
        // Distinct colors for different metrics
        cpuColor = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);      // Blue
        memoryColor = ImVec4(0.80f, 0.40f, 0.80f, 1.00f);   // Purple
        diskColor = ImVec4(1.00f, 0.65f, 0.00f, 1.00f);     // Orange
        temperatureColor = ImVec4(0.90f, 0.20f, 0.20f, 1.00f); // Red
        networkRxColor = ImVec4(0.20f, 0.80f, 0.20f, 1.00f);   // Green
        networkTxColor = ImVec4(0.20f, 0.80f, 0.80f, 1.00f);   // Cyan
        
        textPrimary = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
        textSecondary = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        textMuted = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        background = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        backgroundSecondary = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        border = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    }
};

// UI layout and responsive design
struct UILayout {
    ImVec2 windowSize;
    bool isCompact;
    float contentWidth;
    float sidebarWidth;
    float graphHeight;
    int columnsCount;
    
    UILayout() {
        windowSize = ImVec2(1280, 720);
        isCompact = false;
        contentWidth = 1200;
        sidebarWidth = 250;
        graphHeight = 200;
        columnsCount = 2;
    }
    
    void updateLayout(ImVec2 newSize) {
        windowSize = newSize;
        isCompact = newSize.x < 1000 || newSize.y < 600;
        contentWidth = newSize.x - 40;
        sidebarWidth = isCompact ? 200 : 250;
        graphHeight = isCompact ? 150 : 200;
        columnsCount = isCompact ? 1 : 2;
    }
};

// Enhanced UI helper functions
void setupUITheme();
void setupUIColors(const UIColorScheme& colors);
ImVec4 getResourceColor(float value, float warningThreshold = 80.0f, float criticalThreshold = 95.0f);
ImVec4 getTrendColor(float trend);
void renderSectionHeader(const char* title, const char* icon = nullptr);
void renderMetricCard(const char* title, const char* value, const char* unit, ImVec4 color, const char* tooltip = nullptr);
void renderProgressBar(const char* label, float value, float max_value, ImVec4 color, const char* tooltip = nullptr);
void renderEnhancedGraph(const char* title, const vector<float>& data, ImVec4 color, float minValue, float maxValue, const char* tooltip = nullptr);
void renderStatusBadge(const char* text, ImVec4 color);
void renderTooltip(const char* text);
void renderHelpMarker(const char* desc);
void beginResponsiveLayout(int columns = 2);
void nextResponsiveColumn();
void endResponsiveLayout();
void renderEnhancedMetricDisplay(const char* title, float currentValue, float averageValue, 
                                float maxValue, const char* unit, ImVec4 color,
                                float warningThreshold = 80.0f, float criticalThreshold = 95.0f);

// Global UI state
extern UIColorScheme g_colorScheme;
extern UILayout g_layout;

// student TODO : memory and processes

// student TODO : network

#endif

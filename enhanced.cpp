#include "header.h"
#include <iomanip>
#include <cmath>

// Global historical data instance
static HistoricalData g_historicalData;

// ===== HISTORICAL DATA MANAGEMENT =====

void initializeHistoricalData(HistoricalData& data)
{
    data.startTime = std::chrono::steady_clock::now();
    data.totalDataPoints = 0;
    
    // Load configuration if it exists
    loadConfiguration(data.config);
    
    // Create export directory if needed
    if (data.config.enableDataExport) {
        string createDirCmd = "mkdir -p " + data.config.exportPath;
        system(createDirCmd.c_str());
    }
}

void HistoricalData::addDataPoint(const string& metric, float value)
{
    HistoricalDataPoint point(value);
    
    if (metric == "cpu") {
        cpuHistory.push_back(point);
        if (cpuHistory.size() > config.maxHistoryPoints) {
            cpuHistory.pop_front();
        }
    } else if (metric == "memory") {
        memoryHistory.push_back(point);
        if (memoryHistory.size() > config.maxHistoryPoints) {
            memoryHistory.pop_front();
        }
    } else if (metric == "disk") {
        diskHistory.push_back(point);
        if (diskHistory.size() > config.maxHistoryPoints) {
            diskHistory.pop_front();
        }
    } else if (metric == "temperature") {
        temperatureHistory.push_back(point);
        if (temperatureHistory.size() > config.maxHistoryPoints) {
            temperatureHistory.pop_front();
        }
    } else if (metric == "network_rx") {
        networkRxHistory.push_back(point);
        if (networkRxHistory.size() > config.maxHistoryPoints) {
            networkRxHistory.pop_front();
        }
    } else if (metric == "network_tx") {
        networkTxHistory.push_back(point);
        if (networkTxHistory.size() > config.maxHistoryPoints) {
            networkTxHistory.pop_front();
        }
    }
    
    totalDataPoints++;
}

void HistoricalData::trimHistory()
{
    while (cpuHistory.size() > config.maxHistoryPoints) cpuHistory.pop_front();
    while (memoryHistory.size() > config.maxHistoryPoints) memoryHistory.pop_front();
    while (diskHistory.size() > config.maxHistoryPoints) diskHistory.pop_front();
    while (temperatureHistory.size() > config.maxHistoryPoints) temperatureHistory.pop_front();
    while (networkRxHistory.size() > config.maxHistoryPoints) networkRxHistory.pop_front();
    while (networkTxHistory.size() > config.maxHistoryPoints) networkTxHistory.pop_front();
    
    // Trim notifications (keep last 100)
    while (notifications.size() > 100) {
        notifications.pop_front();
    }
}

void updateHistoricalData(HistoricalData& data, const CPUMonitor& cpu, 
                         const MemoryProcessMonitor& memory, const ThermalMonitor& thermal,
                         const NetworkMonitor& network)
{
    // Add current values to history
    data.addDataPoint("cpu", cpu.currentCPUPercent);
    data.addDataPoint("memory", memory.memory.memUsedPercent);
    data.addDataPoint("disk", memory.disk.usedPercent);
    data.addDataPoint("temperature", thermal.currentMaxTemp);
    
    // Calculate network speeds
    float totalRx = 0.0f, totalTx = 0.0f;
    for (const auto& iface : network.interfaces) {
        if (iface.name != "lo") {
            totalRx += iface.rxSpeed;
            totalTx += iface.txSpeed;
        }
    }
    data.addDataPoint("network_rx", totalRx);
    data.addDataPoint("network_tx", totalTx);
    
    // Update alerts
    if (data.config.enableAlerts) {
        updateAlerts(data, cpu, memory, thermal);
    }
    
    // Update trend analysis
    if (data.config.enableTrendAnalysis) {
        updateTrendAnalysis(data);
    }
    
    // Trim history to maintain size limits
    data.trimHistory();
}

// ===== ALERT SYSTEM =====

void updateAlerts(HistoricalData& data, const CPUMonitor& cpu, 
                 const MemoryProcessMonitor& memory, const ThermalMonitor& thermal)
{
    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastCheck = std::chrono::duration_cast<std::chrono::seconds>(
        now - data.config.cpuAlert.lastTriggered).count();
    
    // CPU Alert
    if (data.config.cpuAlert.enabled && timeSinceLastCheck > 30) {
        if (cpu.currentCPUPercent >= data.config.cpuAlert.criticalLevel) {
            AlertNotification alert("CPU Critical", 
                formatAlertMessage(data.config.cpuAlert, cpu.currentCPUPercent), "critical");
            data.addNotification(alert);
            data.config.cpuAlert.lastTriggered = now;
            data.config.cpuAlert.isActive = true;
        } else if (cpu.currentCPUPercent >= data.config.cpuAlert.warningLevel) {
            AlertNotification alert("CPU Warning", 
                formatAlertMessage(data.config.cpuAlert, cpu.currentCPUPercent), "warning");
            data.addNotification(alert);
            data.config.cpuAlert.lastTriggered = now;
            data.config.cpuAlert.isActive = true;
        } else {
            data.config.cpuAlert.isActive = false;
        }
    }
    
    // Memory Alert
    timeSinceLastCheck = std::chrono::duration_cast<std::chrono::seconds>(
        now - data.config.memoryAlert.lastTriggered).count();
    
    if (data.config.memoryAlert.enabled && timeSinceLastCheck > 30) {
        if (memory.memory.memUsedPercent >= data.config.memoryAlert.criticalLevel) {
            AlertNotification alert("Memory Critical", 
                formatAlertMessage(data.config.memoryAlert, memory.memory.memUsedPercent), "critical");
            data.addNotification(alert);
            data.config.memoryAlert.lastTriggered = now;
            data.config.memoryAlert.isActive = true;
        } else if (memory.memory.memUsedPercent >= data.config.memoryAlert.warningLevel) {
            AlertNotification alert("Memory Warning", 
                formatAlertMessage(data.config.memoryAlert, memory.memory.memUsedPercent), "warning");
            data.addNotification(alert);
            data.config.memoryAlert.lastTriggered = now;
            data.config.memoryAlert.isActive = true;
        } else {
            data.config.memoryAlert.isActive = false;
        }
    }
    
    // Temperature Alert
    timeSinceLastCheck = std::chrono::duration_cast<std::chrono::seconds>(
        now - data.config.temperatureAlert.lastTriggered).count();
    
    if (data.config.temperatureAlert.enabled && timeSinceLastCheck > 30) {
        if (thermal.currentMaxTemp >= data.config.temperatureAlert.criticalLevel) {
            AlertNotification alert("Temperature Critical", 
                formatAlertMessage(data.config.temperatureAlert, thermal.currentMaxTemp), "critical");
            data.addNotification(alert);
            data.config.temperatureAlert.lastTriggered = now;
            data.config.temperatureAlert.isActive = true;
        } else if (thermal.currentMaxTemp >= data.config.temperatureAlert.warningLevel) {
            AlertNotification alert("Temperature Warning", 
                formatAlertMessage(data.config.temperatureAlert, thermal.currentMaxTemp), "warning");
            data.addNotification(alert);
            data.config.temperatureAlert.lastTriggered = now;
            data.config.temperatureAlert.isActive = true;
        } else {
            data.config.temperatureAlert.isActive = false;
        }
    }
}

string formatAlertMessage(const AlertThreshold& alert, float currentValue)
{
    std::ostringstream oss;
    oss << alert.name << " is at " << std::fixed << std::setprecision(1) << currentValue;
    
    if (currentValue >= alert.criticalLevel) {
        oss << "% (Critical threshold: " << alert.criticalLevel << "%)";
    } else if (currentValue >= alert.warningLevel) {
        oss << "% (Warning threshold: " << alert.warningLevel << "%)";
    }
    
    return oss.str();
}

void HistoricalData::addNotification(const AlertNotification& notification)
{
    notifications.push_back(notification);
    
    // Print to console for immediate attention
    std::cout << "[" << formatTimestamp(notification.timestamp) << "] " 
              << notification.severity << ": " << notification.title 
              << " - " << notification.message << std::endl;
}

// ===== TREND ANALYSIS =====

TrendAnalysis performTrendAnalysis(const deque<HistoricalDataPoint>& history, 
                                  const string& metricName)
{
    TrendAnalysis result;
    result.metric = metricName;
    
    if (history.empty()) {
        return result;
    }
    
    result.currentValue = history.back().value;
    
    // Calculate average
    float sum = 0.0f;
    for (const auto& point : history) {
        sum += point.value;
    }
    result.averageValue = sum / history.size();
    
    // Calculate trend using linear regression
    if (history.size() > 1) {
        float n = history.size();
        float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumX2 = 0.0f;
        
        for (size_t i = 0; i < history.size(); ++i) {
            float x = i;
            float y = history[i].value;
            sumX += x;
            sumY += y;
            sumXY += x * y;
            sumX2 += x * x;
        }
        
        // Calculate slope (trend)
        float denominator = n * sumX2 - sumX * sumX;
        if (denominator != 0) {
            result.trend = (n * sumXY - sumX * sumY) / denominator;
        }
        
        // Simple prediction: current + trend
        result.prediction = result.currentValue + result.trend;
    }
    
    // Generate human-readable analysis
    std::ostringstream analysis;
    if (abs(result.trend) < 0.1f) {
        analysis << "Stable";
    } else if (result.trend > 0) {
        analysis << "Increasing (" << std::fixed << std::setprecision(2) << result.trend << "/min)";
    } else {
        analysis << "Decreasing (" << std::fixed << std::setprecision(2) << -result.trend << "/min)";
    }
    
    // Check for anomalies (value significantly different from average)
    float stdDev = calculateStandardDeviation(history);
    if (abs(result.currentValue - result.averageValue) > 2 * stdDev) {
        result.isAnomalous = true;
        analysis << " - ANOMALY DETECTED";
    }
    
    result.analysis = analysis.str();
    return result;
}

void updateTrendAnalysis(HistoricalData& data)
{
    data.trendResults.clear();
    
    data.trendResults.push_back(performTrendAnalysis(data.cpuHistory, "CPU Usage"));
    data.trendResults.push_back(performTrendAnalysis(data.memoryHistory, "Memory Usage"));
    data.trendResults.push_back(performTrendAnalysis(data.diskHistory, "Disk Usage"));
    data.trendResults.push_back(performTrendAnalysis(data.temperatureHistory, "Temperature"));
    data.trendResults.push_back(performTrendAnalysis(data.networkRxHistory, "Network RX"));
    data.trendResults.push_back(performTrendAnalysis(data.networkTxHistory, "Network TX"));
}

// ===== UTILITY FUNCTIONS =====

string formatDuration(std::chrono::steady_clock::duration duration)
{
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration % std::chrono::hours(1));
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration % std::chrono::minutes(1));
    
    std::ostringstream oss;
    if (hours.count() > 0) {
        oss << hours.count() << "h ";
    }
    if (minutes.count() > 0) {
        oss << minutes.count() << "m ";
    }
    oss << seconds.count() << "s";
    
    return oss.str();
}

string formatTimestamp(std::chrono::steady_clock::time_point timestamp)
{
    auto time_t = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now() + 
        (timestamp - std::chrono::steady_clock::now()));
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    return oss.str();
}

float calculateMovingAverage(const deque<HistoricalDataPoint>& history, int periods)
{
    if (history.empty() || periods <= 0) {
        return 0.0f;
    }
    
    int start = std::max(0, (int)history.size() - periods);
    float sum = 0.0f;
    int count = 0;
    
    for (int i = start; i < history.size(); ++i) {
        sum += history[i].value;
        count++;
    }
    
    return count > 0 ? sum / count : 0.0f;
}

float calculateStandardDeviation(const deque<HistoricalDataPoint>& history)
{
    if (history.size() < 2) {
        return 0.0f;
    }
    
    float mean = calculateMovingAverage(history, history.size());
    float variance = 0.0f;
    
    for (const auto& point : history) {
        variance += (point.value - mean) * (point.value - mean);
    }
    
    variance /= history.size();
    return sqrt(variance);
}

// Global accessor for historical data
HistoricalData& getHistoricalData()
{
    return g_historicalData;
}
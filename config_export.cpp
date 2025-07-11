#include "header.h"
#include <iomanip>

// ===== CONFIGURATION MANAGEMENT =====

bool loadConfiguration(SystemConfig& config, const string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        // Create default config file
        return saveConfiguration(config, filename);
    }
    
    string line;
    while (std::getline(file, line)) {
        // Simple key=value parsing
        size_t pos = line.find('=');
        if (pos != string::npos) {
            string key = line.substr(0, pos);
            string value = line.substr(pos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            if (key == "refreshRate") {
                config.refreshRate = std::stof(value);
            } else if (key == "maxHistoryPoints") {
                config.maxHistoryPoints = std::stoi(value);
            } else if (key == "enableAlerts") {
                config.enableAlerts = (value == "true" || value == "1");
            } else if (key == "enableTrendAnalysis") {
                config.enableTrendAnalysis = (value == "true" || value == "1");
            } else if (key == "enableDataExport") {
                config.enableDataExport = (value == "true" || value == "1");
            } else if (key == "exportPath") {
                config.exportPath = value;
            } else if (key == "exportFormat") {
                config.exportFormat = value;
            } else if (key == "cpuWarning") {
                config.cpuAlert.warningLevel = std::stof(value);
            } else if (key == "cpuCritical") {
                config.cpuAlert.criticalLevel = std::stof(value);
            } else if (key == "memoryWarning") {
                config.memoryAlert.warningLevel = std::stof(value);
            } else if (key == "memoryCritical") {
                config.memoryAlert.criticalLevel = std::stof(value);
            } else if (key == "temperatureWarning") {
                config.temperatureAlert.warningLevel = std::stof(value);
            } else if (key == "temperatureCritical") {
                config.temperatureAlert.criticalLevel = std::stof(value);
            }
        }
    }
    
    file.close();
    return true;
}

bool saveConfiguration(const SystemConfig& config, const string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << "# System Monitor Configuration\n";
    file << "refreshRate=" << config.refreshRate << "\n";
    file << "maxHistoryPoints=" << config.maxHistoryPoints << "\n";
    file << "enableAlerts=" << (config.enableAlerts ? "true" : "false") << "\n";
    file << "enableTrendAnalysis=" << (config.enableTrendAnalysis ? "true" : "false") << "\n";
    file << "enableDataExport=" << (config.enableDataExport ? "true" : "false") << "\n";
    file << "exportPath=" << config.exportPath << "\n";
    file << "exportFormat=" << config.exportFormat << "\n";
    file << "trendAnalysisPeriod=" << config.trendAnalysisPeriod << "\n";
    file << "trendSensitivity=" << config.trendSensitivity << "\n";
    
    file << "\n# Alert Thresholds\n";
    file << "cpuWarning=" << config.cpuAlert.warningLevel << "\n";
    file << "cpuCritical=" << config.cpuAlert.criticalLevel << "\n";
    file << "memoryWarning=" << config.memoryAlert.warningLevel << "\n";
    file << "memoryCritical=" << config.memoryAlert.criticalLevel << "\n";
    file << "temperatureWarning=" << config.temperatureAlert.warningLevel << "\n";
    file << "temperatureCritical=" << config.temperatureAlert.criticalLevel << "\n";
    
    file.close();
    return true;
}

// ===== EXPORT FUNCTIONALITY =====

bool exportToCSV(const HistoricalData& data, const string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // Write header
    file << "Timestamp,CPU,Memory,Disk,Temperature,Network_RX,Network_TX\\n";
    
    // Find the minimum size across all histories
    size_t minSize = std::min({
        data.cpuHistory.size(),
        data.memoryHistory.size(),
        data.diskHistory.size(),
        data.temperatureHistory.size(),
        data.networkRxHistory.size(),
        data.networkTxHistory.size()
    });
    
    // Write data
    for (size_t i = 0; i < minSize; ++i) {
        size_t idx = data.cpuHistory.size() - minSize + i;
        
        // Format timestamp
        auto timestamp = data.cpuHistory[idx].timestamp;
        auto time_t = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now() + 
            (timestamp - std::chrono::steady_clock::now()));
        
        file << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << ",";
        file << data.cpuHistory[idx].value << ",";
        file << data.memoryHistory[idx].value << ",";
        file << data.diskHistory[idx].value << ",";
        file << data.temperatureHistory[idx].value << ",";
        file << data.networkRxHistory[idx].value << ",";
        file << data.networkTxHistory[idx].value << "\\n";
    }
    
    file.close();
    return true;
}

bool exportToJSON(const HistoricalData& data, const string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << "{\n";
    
    // Export metadata
    auto uptime = std::chrono::steady_clock::now() - data.startTime;
    auto uptimeSeconds = std::chrono::duration_cast<std::chrono::seconds>(uptime).count();
    file << "  \"metadata\": {\n";
    file << "    \"uptime_seconds\": " << uptimeSeconds << ",\n";
    file << "    \"total_data_points\": " << data.totalDataPoints << ",\n";
    file << "    \"export_timestamp\": " << 
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() << "\n";
    file << "  },\n";
    
    // Export CPU data
    file << "  \"cpu_data\": [\n";
    for (size_t i = 0; i < data.cpuHistory.size(); ++i) {
        const auto& point = data.cpuHistory[i];
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            point.timestamp - data.startTime).count();
        file << "    {\"elapsed_seconds\": " << elapsed << ", \"value\": " << point.value << "}";
        if (i < data.cpuHistory.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ],\n";
    
    // Export trend analysis
    file << "  \"trends\": [\n";
    for (size_t i = 0; i < data.trendResults.size(); ++i) {
        const auto& trend = data.trendResults[i];
        file << "    {\n";
        file << "      \"metric\": \"" << trend.metric << "\",\n";
        file << "      \"current_value\": " << trend.currentValue << ",\n";
        file << "      \"average_value\": " << trend.averageValue << ",\n";
        file << "      \"trend\": " << trend.trend << ",\n";
        file << "      \"analysis\": \"" << trend.analysis << "\",\n";
        file << "      \"is_anomalous\": " << (trend.isAnomalous ? "true" : "false") << "\n";
        file << "    }";
        if (i < data.trendResults.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ]\n";
    
    file << "}\n";
    file.close();
    
    return true;
}

void HistoricalData::exportData(const string& format, const string& filename)
{
    if (format == "csv") {
        exportToCSV(*this, filename);
    } else if (format == "json") {
        exportToJSON(*this, filename);
    }
}
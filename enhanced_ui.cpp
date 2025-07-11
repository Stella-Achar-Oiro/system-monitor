#include "header.h"
#include <iomanip>

// ===== ENHANCED UI COMPONENTS =====

void renderConfigurationInterface(SystemConfig& config)
{
    renderSectionHeader("Configuration", "⚙");
    
    beginResponsiveLayout(2);
    
    // Left column - Display & Features
    {
        
        // Display Settings
        ImGui::Text("Display Settings");
        ImGui::Separator();
        
        ImGui::SliderFloat("Refresh Rate (FPS)", &config.refreshRate, 1.0f, 120.0f, "%.1f");
        ImGui::SliderInt("Max History Points", &config.maxHistoryPoints, 100, 10000);
        
        ImGui::Spacing();
        
        // Feature Toggles
        ImGui::Text("Features");
        ImGui::Separator();
        
        ImGui::Checkbox("Enable Alerts", &config.enableAlerts);
        ImGui::Checkbox("Enable Trend Analysis", &config.enableTrendAnalysis);
        ImGui::Checkbox("Enable Data Export", &config.enableDataExport);
        
        ImGui::Spacing();
        
        // Alert Thresholds
        if (config.enableAlerts) {
            ImGui::Text("Alert Thresholds");
            ImGui::Separator();
            
            ImGui::Text("CPU Usage");
            ImGui::SliderFloat("CPU Warning##cpu_warn", &config.cpuAlert.warningLevel, 50.0f, 100.0f, "%.1f%%");
            ImGui::SliderFloat("CPU Critical##cpu_crit", &config.cpuAlert.criticalLevel, 50.0f, 100.0f, "%.1f%%");
            ImGui::Checkbox("CPU Alerts Enabled##cpu_en", &config.cpuAlert.enabled);
            
            ImGui::Text("Memory Usage");
            ImGui::SliderFloat("Memory Warning##mem_warn", &config.memoryAlert.warningLevel, 50.0f, 100.0f, "%.1f%%");
            ImGui::SliderFloat("Memory Critical##mem_crit", &config.memoryAlert.criticalLevel, 50.0f, 100.0f, "%.1f%%");
            ImGui::Checkbox("Memory Alerts Enabled##mem_en", &config.memoryAlert.enabled);
            
            ImGui::Text("Temperature");
            ImGui::SliderFloat("Temperature Warning##temp_warn", &config.temperatureAlert.warningLevel, 40.0f, 100.0f, "%.1f°C");
            ImGui::SliderFloat("Temperature Critical##temp_crit", &config.temperatureAlert.criticalLevel, 40.0f, 100.0f, "%.1f°C");
            ImGui::Checkbox("Temperature Alerts Enabled##temp_en", &config.temperatureAlert.enabled);
        }
        
        ImGui::Spacing();
        
        // Export Settings
        if (config.enableDataExport) {
            ImGui::Text("Export Settings");
            ImGui::Separator();
            
            char pathBuffer[512];
            strncpy(pathBuffer, config.exportPath.c_str(), sizeof(pathBuffer) - 1);
            pathBuffer[sizeof(pathBuffer) - 1] = '\0';
            
            if (ImGui::InputText("Export Path", pathBuffer, sizeof(pathBuffer))) {
                config.exportPath = pathBuffer;
            }
            
            // Export format radio buttons
            static int formatIndex = 0;
            if (config.exportFormat == "csv") formatIndex = 0;
            else if (config.exportFormat == "json") formatIndex = 1;
            
            ImGui::RadioButton("CSV", &formatIndex, 0); ImGui::SameLine();
            ImGui::RadioButton("JSON", &formatIndex, 1);
            
            config.exportFormat = (formatIndex == 0) ? "csv" : "json";
        }
        
        ImGui::Spacing();
        
        // Save/Load buttons
        if (ImGui::Button("Save Configuration")) {
            if (saveConfiguration(config, "monitor_config.conf")) {
                // Show success message briefly
                ImGui::OpenPopup("Config Saved");
            }
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Load Configuration")) {
            loadConfiguration(config, "monitor_config.conf");
        }
        
        // Success popup
        if (ImGui::BeginPopupModal("Config Saved", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Configuration saved successfully!");
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}

void renderAlertsInterface(HistoricalData& data)
{
    if (ImGui::CollapsingHeader("Alerts & Notifications", ImGuiTreeNodeFlags_DefaultOpen)) {
        
        // Active alerts status
        ImGui::Text("Alert Status");
        ImGui::Separator();
        
        // CPU Alert
        ImVec4 cpuColor = data.config.cpuAlert.isActive ? 
            (data.config.cpuAlert.isActive ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 0.0f, 1.0f)) : 
            ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        
        ImGui::TextColored(cpuColor, "CPU: %s", data.config.cpuAlert.isActive ? "ACTIVE" : "OK");
        ImGui::SameLine();
        ImGui::Text("(W: %.1f%%, C: %.1f%%)", data.config.cpuAlert.warningLevel, data.config.cpuAlert.criticalLevel);
        
        // Memory Alert
        ImVec4 memColor = data.config.memoryAlert.isActive ? 
            ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        
        ImGui::TextColored(memColor, "Memory: %s", data.config.memoryAlert.isActive ? "ACTIVE" : "OK");
        ImGui::SameLine();
        ImGui::Text("(W: %.1f%%, C: %.1f%%)", data.config.memoryAlert.warningLevel, data.config.memoryAlert.criticalLevel);
        
        // Temperature Alert
        ImVec4 tempColor = data.config.temperatureAlert.isActive ? 
            ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        
        ImGui::TextColored(tempColor, "Temperature: %s", data.config.temperatureAlert.isActive ? "ACTIVE" : "OK");
        ImGui::SameLine();
        ImGui::Text("(W: %.1f°C, C: %.1f°C)", data.config.temperatureAlert.warningLevel, data.config.temperatureAlert.criticalLevel);
        
        ImGui::Spacing();
        
        // Recent notifications
        ImGui::Text("Recent Notifications (%d)", (int)data.notifications.size());
        ImGui::Separator();
        
        if (data.notifications.empty()) {
            ImGui::Text("No notifications");
        } else {
            if (ImGui::BeginChild("NotificationsList", ImVec2(0, 150), true)) {
                for (auto it = data.notifications.rbegin(); it != data.notifications.rend(); ++it) {
                    const auto& notification = *it;
                    
                    ImVec4 severityColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                    if (notification.severity == "critical") {
                        severityColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                    } else if (notification.severity == "warning") {
                        severityColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                    }
                    
                    ImGui::TextColored(severityColor, "[%s] %s", 
                                     formatTimestamp(notification.timestamp).c_str(),
                                     notification.severity.c_str());
                    ImGui::Text("  %s: %s", notification.title.c_str(), notification.message.c_str());
                    ImGui::Separator();
                }
            }
            ImGui::EndChild();
            
            if (ImGui::Button("Clear Notifications")) {
                data.notifications.clear();
            }
        }
    }
}

void renderTrendAnalysisInterface(const HistoricalData& data)
{
    if (ImGui::CollapsingHeader("Trend Analysis", ImGuiTreeNodeFlags_DefaultOpen)) {
        
        if (data.trendResults.empty()) {
            ImGui::Text("No trend data available");
            return;
        }
        
        ImGui::Text("System Resource Trends");
        ImGui::Separator();
        
        if (ImGui::BeginTable("TrendTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Metric", ImGuiTableColumnFlags_WidthFixed, 120);
            ImGui::TableSetupColumn("Current", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Average", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Trend", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("Analysis", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            
            for (const auto& trend : data.trendResults) {
                ImGui::TableNextRow();
                
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", trend.metric.c_str());
                
                ImGui::TableSetColumnIndex(1);
                if (trend.metric.find("Network") != string::npos) {
                    ImGui::Text("%.2f MB/s", trend.currentValue / (1024 * 1024));
                } else if (trend.metric == "Temperature") {
                    ImGui::Text("%.1f°C", trend.currentValue);
                } else {
                    ImGui::Text("%.1f%%", trend.currentValue);
                }
                
                ImGui::TableSetColumnIndex(2);
                if (trend.metric.find("Network") != string::npos) {
                    ImGui::Text("%.2f MB/s", trend.averageValue / (1024 * 1024));
                } else if (trend.metric == "Temperature") {
                    ImGui::Text("%.1f°C", trend.averageValue);
                } else {
                    ImGui::Text("%.1f%%", trend.averageValue);
                }
                
                ImGui::TableSetColumnIndex(3);
                ImVec4 trendColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                if (trend.trend > 0.1f) {
                    trendColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red for increasing
                } else if (trend.trend < -0.1f) {
                    trendColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for decreasing
                }
                ImGui::TextColored(trendColor, "%.2f", trend.trend);
                
                ImGui::TableSetColumnIndex(4);
                if (trend.isAnomalous) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", trend.analysis.c_str());
                } else {
                    ImGui::Text("%s", trend.analysis.c_str());
                }
            }
            
            ImGui::EndTable();
        }
        
        ImGui::Spacing();
        ImGui::Text("Trend Analysis Period: %d minutes", data.config.trendAnalysisPeriod);
        ImGui::Text("Total Data Points: %lld", data.totalDataPoints);
        
        auto uptime = std::chrono::steady_clock::now() - data.startTime;
        ImGui::Text("Monitoring Uptime: %s", formatDuration(uptime).c_str());
    }
}

void renderExportInterface(HistoricalData& data)
{
    if (ImGui::CollapsingHeader("Data Export", ImGuiTreeNodeFlags_DefaultOpen)) {
        
        if (!data.config.enableDataExport) {
            ImGui::Text("Data export is disabled in configuration");
            return;
        }
        
        ImGui::Text("Export System Data");
        ImGui::Separator();
        
        // Export statistics
        ImGui::Text("Available Data:");
        ImGui::BulletText("CPU History: %d points", (int)data.cpuHistory.size());
        ImGui::BulletText("Memory History: %d points", (int)data.memoryHistory.size());
        ImGui::BulletText("Temperature History: %d points", (int)data.temperatureHistory.size());
        ImGui::BulletText("Network History: %d points", (int)data.networkRxHistory.size());
        
        ImGui::Spacing();
        
        // Export format selection
        static int exportFormat = 0;
        ImGui::Text("Export Format:");
        ImGui::RadioButton("CSV", &exportFormat, 0); ImGui::SameLine();
        ImGui::RadioButton("JSON", &exportFormat, 1);
        
        ImGui::Spacing();
        
        // Export filename
        static char exportFilename[256] = "system_monitor_data";
        ImGui::InputText("Filename (without extension)", exportFilename, sizeof(exportFilename));
        
        ImGui::Spacing();
        
        // Export buttons
        if (ImGui::Button("Export Data")) {
            string format = (exportFormat == 0) ? "csv" : "json";
            string extension = (exportFormat == 0) ? ".csv" : ".json";
            string fullFilename = data.config.exportPath + exportFilename + extension;
            
            bool success = false;
            if (format == "csv") {
                success = exportToCSV(data, fullFilename);
            } else {
                success = exportToJSON(data, fullFilename);
            }
            
            if (success) {
                ImGui::OpenPopup("Export Success");
            } else {
                ImGui::OpenPopup("Export Failed");
            }
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Open Export Directory")) {
            string openCmd = "xdg-open " + data.config.exportPath;
            system(openCmd.c_str());
        }
        
        // Export result popups
        if (ImGui::BeginPopupModal("Export Success", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Data exported successfully!");
            ImGui::Text("Location: %s", (data.config.exportPath + exportFilename + 
                       ((exportFormat == 0) ? ".csv" : ".json")).c_str());
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        if (ImGui::BeginPopupModal("Export Failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Export failed!");
            ImGui::Text("Please check the export path and permissions.");
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}

void renderHistoricalGraphs(const HistoricalData& data)
{
    if (ImGui::CollapsingHeader("Historical Graphs", ImGuiTreeNodeFlags_DefaultOpen)) {
        
        // CPU History Graph
        if (!data.cpuHistory.empty()) {
            vector<float> cpuValues;
            for (const auto& point : data.cpuHistory) {
                cpuValues.push_back(point.value);
            }
            
            ImGui::Text("CPU Usage History");
            ImGui::PlotLines("CPU%%", cpuValues.data(), cpuValues.size(), 
                           0, nullptr, 0.0f, 100.0f, ImVec2(0, 100));
        }
        
        // Memory History Graph
        if (!data.memoryHistory.empty()) {
            vector<float> memValues;
            for (const auto& point : data.memoryHistory) {
                memValues.push_back(point.value);
            }
            
            ImGui::Text("Memory Usage History");
            ImGui::PlotLines("Memory%%", memValues.data(), memValues.size(), 
                           0, nullptr, 0.0f, 100.0f, ImVec2(0, 100));
        }
        
        // Temperature History Graph
        if (!data.temperatureHistory.empty()) {
            vector<float> tempValues;
            for (const auto& point : data.temperatureHistory) {
                tempValues.push_back(point.value);
            }
            
            ImGui::Text("Temperature History");
            ImGui::PlotLines("Temp°C", tempValues.data(), tempValues.size(), 
                           0, nullptr, 20.0f, 100.0f, ImVec2(0, 100));
        }
    }
}

void renderAdvancedSystemOverview(const HistoricalData& data, const CPUMonitor& cpu,
                                const MemoryProcessMonitor& memory, const ThermalMonitor& thermal)
{
    if (ImGui::CollapsingHeader("Advanced System Overview", ImGuiTreeNodeFlags_DefaultOpen)) {
        
        // System status indicators
        ImGui::Text("System Health Status");
        ImGui::Separator();
        
        // Overall health indicator
        bool anyAlerts = data.config.cpuAlert.isActive || 
                        data.config.memoryAlert.isActive || 
                        data.config.temperatureAlert.isActive;
        
        ImVec4 healthColor = anyAlerts ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        ImGui::TextColored(healthColor, "Overall Status: %s", anyAlerts ? "ALERT" : "HEALTHY");
        
        ImGui::Spacing();
        
        // Quick stats with trends
        ImGui::Columns(3, "QuickStats", true);
        
        // CPU Column
        ImGui::Text("CPU");
        ImGui::Separator();
        ImGui::Text("Current: %.1f%%", cpu.currentCPUPercent);
        
        if (!data.trendResults.empty()) {
            auto cpuTrend = std::find_if(data.trendResults.begin(), data.trendResults.end(),
                [](const TrendAnalysis& t) { return t.metric == "CPU Usage"; });
            if (cpuTrend != data.trendResults.end()) {
                ImGui::Text("Average: %.1f%%", cpuTrend->averageValue);
                ImVec4 trendColor = (cpuTrend->trend > 0) ? ImVec4(1.0f, 0.5f, 0.0f, 1.0f) : 
                                   (cpuTrend->trend < 0) ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                ImGui::TextColored(trendColor, "Trend: %.2f", cpuTrend->trend);
            }
        }
        
        ImGui::NextColumn();
        
        // Memory Column
        ImGui::Text("Memory");
        ImGui::Separator();
        ImGui::Text("Current: %.1f%%", memory.memory.memUsedPercent);
        ImGui::Text("Available: %s", formatBytes(memory.memory.memAvailable * 1024).c_str());
        
        if (!data.trendResults.empty()) {
            auto memTrend = std::find_if(data.trendResults.begin(), data.trendResults.end(),
                [](const TrendAnalysis& t) { return t.metric == "Memory Usage"; });
            if (memTrend != data.trendResults.end()) {
                ImGui::Text("Average: %.1f%%", memTrend->averageValue);
                ImVec4 trendColor = (memTrend->trend > 0) ? ImVec4(1.0f, 0.5f, 0.0f, 1.0f) : 
                                   (memTrend->trend < 0) ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                ImGui::TextColored(trendColor, "Trend: %.2f", memTrend->trend);
            }
        }
        
        ImGui::NextColumn();
        
        // Temperature Column
        ImGui::Text("Temperature");
        ImGui::Separator();
        ImGui::Text("Max: %.1f°C", thermal.currentMaxTemp);
        
        if (!data.trendResults.empty()) {
            auto tempTrend = std::find_if(data.trendResults.begin(), data.trendResults.end(),
                [](const TrendAnalysis& t) { return t.metric == "Temperature"; });
            if (tempTrend != data.trendResults.end()) {
                ImGui::Text("Average: %.1f°C", tempTrend->averageValue);
                ImVec4 trendColor = (tempTrend->trend > 0) ? ImVec4(1.0f, 0.5f, 0.0f, 1.0f) : 
                                   (tempTrend->trend < 0) ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                ImGui::TextColored(trendColor, "Trend: %.2f", tempTrend->trend);
            }
        }
        
        ImGui::Columns(1);
        
        ImGui::Spacing();
        
        // Monitoring statistics
        ImGui::Text("Monitoring Statistics");
        ImGui::Separator();
        
        auto uptime = std::chrono::steady_clock::now() - data.startTime;
        ImGui::Text("Uptime: %s", formatDuration(uptime).c_str());
        ImGui::Text("Data Points Collected: %lld", data.totalDataPoints);
        ImGui::Text("Recent Alerts: %d", (int)data.notifications.size());
        
        // Quick export button
        if (data.config.enableDataExport) {
            ImGui::Spacing();
            if (ImGui::Button("Quick Export (CSV)")) {
                auto now = std::time(nullptr);
                std::ostringstream filename;
                filename << data.config.exportPath << "quick_export_" 
                        << std::put_time(std::localtime(&now), "%Y%m%d_%H%M%S") << ".csv";
                exportToCSV(data, filename.str());
            }
        }
    }
}
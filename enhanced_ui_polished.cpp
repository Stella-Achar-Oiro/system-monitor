#include "header.h"
#include <iomanip>

// ===== POLISHED UI COMPONENTS =====

void renderConfigurationInterface(SystemConfig& config)
{
    renderSectionHeader("Configuration", "⚙");
    
    beginResponsiveLayout(2);
    
    // Left column - Display & Features
    {
        ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textSecondary);
        ImGui::Text("Display Settings");
        ImGui::PopStyleColor();
        ImGui::Separator();
        
        ImGui::SliderFloat("Refresh Rate (FPS)", &config.refreshRate, 1.0f, 120.0f, "%.1f");
        ImGui::SameLine(); renderHelpMarker("Controls how often the interface updates. Higher values use more CPU.");
        
        ImGui::SliderInt("Max History Points", &config.maxHistoryPoints, 100, 10000);
        ImGui::SameLine(); renderHelpMarker("Maximum number of data points to store in memory for historical graphs.");
        
        ImGui::Spacing();
        
        // Feature Toggles
        ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textSecondary);
        ImGui::Text("Features");
        ImGui::PopStyleColor();
        ImGui::Separator();
        
        ImGui::Checkbox("Enable Alerts", &config.enableAlerts);
        ImGui::SameLine(); renderHelpMarker("Monitor system resources and show alerts when thresholds are exceeded.");
        
        ImGui::Checkbox("Enable Trend Analysis", &config.enableTrendAnalysis);
        ImGui::SameLine(); renderHelpMarker("Analyze resource usage patterns and predict future trends.");
        
        ImGui::Checkbox("Enable Data Export", &config.enableDataExport);
        ImGui::SameLine(); renderHelpMarker("Allow exporting system monitoring data to CSV and JSON formats.");
        
        // Export Settings
        if (config.enableDataExport) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textSecondary);
            ImGui::Text("Export Settings");
            ImGui::PopStyleColor();
            ImGui::Separator();
            
            char pathBuffer[512];
            strncpy(pathBuffer, config.exportPath.c_str(), sizeof(pathBuffer) - 1);
            pathBuffer[sizeof(pathBuffer) - 1] = '\0';
            
            if (ImGui::InputText("Export Path", pathBuffer, sizeof(pathBuffer))) {
                config.exportPath = pathBuffer;
            }
            
            // Export format radio buttons with colors
            static int formatIndex = 0;
            if (config.exportFormat == "csv") formatIndex = 0;
            else if (config.exportFormat == "json") formatIndex = 1;
            
            ImGui::PushStyleColor(ImGuiCol_CheckMark, g_colorScheme.success);
            ImGui::RadioButton("CSV", &formatIndex, 0); 
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_CheckMark, g_colorScheme.info);
            ImGui::RadioButton("JSON", &formatIndex, 1);
            ImGui::PopStyleColor();
            
            config.exportFormat = (formatIndex == 0) ? "csv" : "json";
        }
    }
    
    nextResponsiveColumn();
    
    // Right column - Alert Thresholds
    {
        if (config.enableAlerts) {
            ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textSecondary);
            ImGui::Text("Alert Thresholds");
            ImGui::PopStyleColor();
            ImGui::Separator();
            
            // CPU Alerts Card
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.14f, 0.14f, 0.14f, 1.00f));
            if (ImGui::BeginChild("CPU_Alerts", ImVec2(0, 120), true)) {
                ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.cpuColor);
                ImGui::Text("🖥 CPU Usage");
                ImGui::PopStyleColor();
                
                ImGui::PushStyleColor(ImGuiCol_SliderGrab, g_colorScheme.warning);
                ImGui::SliderFloat("Warning##cpu_warn", &config.cpuAlert.warningLevel, 50.0f, 100.0f, "%.1f%%");
                ImGui::PopStyleColor();
                
                ImGui::PushStyleColor(ImGuiCol_SliderGrab, g_colorScheme.danger);
                ImGui::SliderFloat("Critical##cpu_crit", &config.cpuAlert.criticalLevel, 50.0f, 100.0f, "%.1f%%");
                ImGui::PopStyleColor();
                
                ImGui::Checkbox("Enabled##cpu_en", &config.cpuAlert.enabled);
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
            
            // Memory Alerts Card
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.14f, 0.14f, 0.14f, 1.00f));
            if (ImGui::BeginChild("Memory_Alerts", ImVec2(0, 120), true)) {
                ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.memoryColor);
                ImGui::Text("🧠 Memory Usage");
                ImGui::PopStyleColor();
                
                ImGui::PushStyleColor(ImGuiCol_SliderGrab, g_colorScheme.warning);
                ImGui::SliderFloat("Warning##mem_warn", &config.memoryAlert.warningLevel, 50.0f, 100.0f, "%.1f%%");
                ImGui::PopStyleColor();
                
                ImGui::PushStyleColor(ImGuiCol_SliderGrab, g_colorScheme.danger);
                ImGui::SliderFloat("Critical##mem_crit", &config.memoryAlert.criticalLevel, 50.0f, 100.0f, "%.1f%%");
                ImGui::PopStyleColor();
                
                ImGui::Checkbox("Enabled##mem_en", &config.memoryAlert.enabled);
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
            
            // Temperature Alerts Card
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.14f, 0.14f, 0.14f, 1.00f));
            if (ImGui::BeginChild("Temp_Alerts", ImVec2(0, 120), true)) {
                ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.temperatureColor);
                ImGui::Text("🌡 Temperature");
                ImGui::PopStyleColor();
                
                ImGui::PushStyleColor(ImGuiCol_SliderGrab, g_colorScheme.warning);
                ImGui::SliderFloat("Warning##temp_warn", &config.temperatureAlert.warningLevel, 40.0f, 100.0f, "%.1f°C");
                ImGui::PopStyleColor();
                
                ImGui::PushStyleColor(ImGuiCol_SliderGrab, g_colorScheme.danger);
                ImGui::SliderFloat("Critical##temp_crit", &config.temperatureAlert.criticalLevel, 40.0f, 100.0f, "%.1f°C");
                ImGui::PopStyleColor();
                
                ImGui::Checkbox("Enabled##temp_en", &config.temperatureAlert.enabled);
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textMuted);
            ImGui::Text("Enable alerts to configure thresholds");
            ImGui::PopStyleColor();
        }
    }
    
    endResponsiveLayout();
    
    // Action buttons section
    ImGui::Spacing();
    renderSectionHeader("Actions");
    
    ImGui::PushStyleColor(ImGuiCol_Button, g_colorScheme.success);
    if (ImGui::Button("💾 Save Configuration", ImVec2(180, 35))) {
        if (saveConfiguration(config, "monitor_config.conf")) {
            ImGui::OpenPopup("Config Saved");
        }
    }
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, g_colorScheme.info);
    if (ImGui::Button("📁 Load Configuration", ImVec2(180, 35))) {
        loadConfiguration(config, "monitor_config.conf");
    }
    ImGui::PopStyleColor();
    
    // Enhanced success popup
    if (ImGui::BeginPopupModal("Config Saved", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.success);
        ImGui::Text("✅ Configuration saved successfully!");
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Text("Settings have been written to monitor_config.conf");
        if (ImGui::Button("OK", ImVec2(120, 30))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void renderAlertsInterface(HistoricalData& data)
{
    renderSectionHeader("System Alerts", "🚨");
    
    beginResponsiveLayout(2);
    
    // Left column - Alert Status
    {
        ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textSecondary);
        ImGui::Text("Alert Status");
        ImGui::PopStyleColor();
        ImGui::Separator();
        
        // CPU Alert Status
        ImVec4 cpuColor = data.config.cpuAlert.isActive ? g_colorScheme.danger : g_colorScheme.success;
        renderStatusBadge(data.config.cpuAlert.isActive ? "CPU: ALERT" : "CPU: OK", cpuColor);
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textMuted);
        ImGui::Text("(W: %.1f%%, C: %.1f%%)", data.config.cpuAlert.warningLevel, data.config.cpuAlert.criticalLevel);
        ImGui::PopStyleColor();
        
        // Memory Alert Status
        ImVec4 memColor = data.config.memoryAlert.isActive ? g_colorScheme.danger : g_colorScheme.success;
        renderStatusBadge(data.config.memoryAlert.isActive ? "MEM: ALERT" : "MEM: OK", memColor);
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textMuted);
        ImGui::Text("(W: %.1f%%, C: %.1f%%)", data.config.memoryAlert.warningLevel, data.config.memoryAlert.criticalLevel);
        ImGui::PopStyleColor();
        
        // Temperature Alert Status
        ImVec4 tempColor = data.config.temperatureAlert.isActive ? g_colorScheme.danger : g_colorScheme.success;
        renderStatusBadge(data.config.temperatureAlert.isActive ? "TEMP: ALERT" : "TEMP: OK", tempColor);
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textMuted);
        ImGui::Text("(W: %.1f°C, C: %.1f°C)", data.config.temperatureAlert.warningLevel, data.config.temperatureAlert.criticalLevel);
        ImGui::PopStyleColor();
        
        // Overall health indicator
        bool anyAlerts = data.config.cpuAlert.isActive || 
                        data.config.memoryAlert.isActive || 
                        data.config.temperatureAlert.isActive;
        
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textSecondary);
        ImGui::Text("Overall System Health");
        ImGui::PopStyleColor();
        
        if (anyAlerts) {
            renderStatusBadge("⚠ ATTENTION REQUIRED", g_colorScheme.danger);
        } else {
            renderStatusBadge("✅ SYSTEM HEALTHY", g_colorScheme.success);
        }
    }
    
    nextResponsiveColumn();
    
    // Right column - Recent Notifications
    {
        ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textSecondary);
        ImGui::Text("Recent Notifications (%d)", (int)data.notifications.size());
        ImGui::PopStyleColor();
        ImGui::Separator();
        
        if (data.notifications.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textMuted);
            ImGui::Text("No notifications");
            ImGui::PopStyleColor();
        } else {
            float childHeight = g_layout.isCompact ? 200 : 300;
            if (ImGui::BeginChild("NotificationsList", ImVec2(0, childHeight), true)) {
                for (auto it = data.notifications.rbegin(); it != data.notifications.rend(); ++it) {
                    const auto& notification = *it;
                    
                    ImVec4 severityColor = g_colorScheme.textSecondary;
                    string severityIcon = "ℹ";
                    if (notification.severity == "critical") {
                        severityColor = g_colorScheme.danger;
                        severityIcon = "🚨";
                    } else if (notification.severity == "warning") {
                        severityColor = g_colorScheme.warning;
                        severityIcon = "⚠";
                    }
                    
                    ImGui::PushStyleColor(ImGuiCol_Text, severityColor);
                    ImGui::Text("%s [%s] %s", 
                               severityIcon.c_str(),
                               formatTimestamp(notification.timestamp).c_str(),
                               notification.severity.c_str());
                    ImGui::PopStyleColor();
                    
                    ImGui::Text("  %s: %s", notification.title.c_str(), notification.message.c_str());
                    ImGui::Separator();
                }
            }
            ImGui::EndChild();
            
            ImGui::PushStyleColor(ImGuiCol_Button, g_colorScheme.warning);
            if (ImGui::Button("🗑 Clear Notifications", ImVec2(160, 30))) {
                data.notifications.clear();
            }
            ImGui::PopStyleColor();
        }
    }
    
    endResponsiveLayout();
}

void renderTrendAnalysisInterface(const HistoricalData& data)
{
    renderSectionHeader("Trend Analysis", "📈");
    
    if (data.trendResults.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textMuted);
        ImGui::Text("No trend data available");
        ImGui::PopStyleColor();
        return;
    }
    
    // Enhanced trend table with better styling
    ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, g_colorScheme.primaryDark);
    ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, g_colorScheme.border);
    
    if (ImGui::BeginTable("TrendTable", 6, 
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable)) {
        
        ImGui::TableSetupColumn("📊 Metric", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupColumn("📍 Current", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("📈 Average", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("🔄 Trend", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("🔮 Prediction", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("🧠 Analysis", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        
        for (const auto& trend : data.trendResults) {
            ImGui::TableNextRow();
            
            // Metric name with color coding
            ImGui::TableSetColumnIndex(0);
            ImVec4 metricColor = g_colorScheme.textPrimary;
            if (trend.metric == "CPU Usage") metricColor = g_colorScheme.cpuColor;
            else if (trend.metric == "Memory Usage") metricColor = g_colorScheme.memoryColor;
            else if (trend.metric == "Temperature") metricColor = g_colorScheme.temperatureColor;
            else if (trend.metric.find("Network") != string::npos) metricColor = g_colorScheme.networkRxColor;
            
            ImGui::PushStyleColor(ImGuiCol_Text, metricColor);
            ImGui::Text("%s", trend.metric.c_str());
            ImGui::PopStyleColor();
            
            // Current value
            ImGui::TableSetColumnIndex(1);
            ImVec4 currentColor = getResourceColor(trend.currentValue, 80.0f, 95.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, currentColor);
            if (trend.metric.find("Network") != string::npos) {
                ImGui::Text("%.1f MB/s", trend.currentValue / (1024 * 1024));
            } else if (trend.metric == "Temperature") {
                ImGui::Text("%.1f°C", trend.currentValue);
            } else {
                ImGui::Text("%.1f%%", trend.currentValue);
            }
            ImGui::PopStyleColor();
            
            // Average value
            ImGui::TableSetColumnIndex(2);
            if (trend.metric.find("Network") != string::npos) {
                ImGui::Text("%.1f MB/s", trend.averageValue / (1024 * 1024));
            } else if (trend.metric == "Temperature") {
                ImGui::Text("%.1f°C", trend.averageValue);
            } else {
                ImGui::Text("%.1f%%", trend.averageValue);
            }
            
            // Trend with arrow indicators
            ImGui::TableSetColumnIndex(3);
            ImVec4 trendColor = getTrendColor(trend.trend);
            ImGui::PushStyleColor(ImGuiCol_Text, trendColor);
            string trendIcon = "→";
            if (trend.trend > 0.1f) trendIcon = "↗";
            else if (trend.trend < -0.1f) trendIcon = "↘";
            ImGui::Text("%s %.2f", trendIcon.c_str(), trend.trend);
            ImGui::PopStyleColor();
            
            // Prediction
            ImGui::TableSetColumnIndex(4);
            if (trend.metric.find("Network") != string::npos) {
                ImGui::Text("%.1f MB/s", trend.prediction / (1024 * 1024));
            } else if (trend.metric == "Temperature") {
                ImGui::Text("%.1f°C", trend.prediction);
            } else {
                ImGui::Text("%.1f%%", trend.prediction);
            }
            
            // Analysis with anomaly highlighting
            ImGui::TableSetColumnIndex(5);
            if (trend.isAnomalous) {
                ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.danger);
                ImGui::Text("🔥 %s", trend.analysis.c_str());
                ImGui::PopStyleColor();
            } else {
                ImGui::Text("%s", trend.analysis.c_str());
            }
        }
        
        ImGui::EndTable();
    }
    
    ImGui::PopStyleColor(2);
    
    // Trend summary
    ImGui::Spacing();
    beginResponsiveLayout(3);
    
    renderMetricCard("Analysis Period", 
                    std::to_string(data.config.trendAnalysisPeriod).c_str(), 
                    "min", 
                    g_colorScheme.info,
                    "Time period used for trend analysis calculations");
    
    nextResponsiveColumn();
    
    renderMetricCard("Data Points", 
                    std::to_string(data.totalDataPoints).c_str(), 
                    "", 
                    g_colorScheme.primary,
                    "Total number of data points collected since monitoring started");
    
    nextResponsiveColumn();
    
    auto uptime = std::chrono::steady_clock::now() - data.startTime;
    renderMetricCard("Uptime", 
                    formatDuration(uptime).c_str(), 
                    "", 
                    g_colorScheme.success,
                    "How long the system monitor has been running");
    
    endResponsiveLayout();
}

void renderExportInterface(HistoricalData& data)
{
    renderSectionHeader("Data Export", "📤");
    
    if (!data.config.enableDataExport) {
        ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.warning);
        ImGui::Text("⚠ Data export is disabled in configuration");
        ImGui::PopStyleColor();
        return;
    }
    
    beginResponsiveLayout(2);
    
    // Left column - Export Statistics
    {
        ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textSecondary);
        ImGui::Text("Available Data");
        ImGui::PopStyleColor();
        ImGui::Separator();
        
        renderMetricCard("CPU History", 
                        std::to_string((int)data.cpuHistory.size()).c_str(), 
                        "points", 
                        g_colorScheme.cpuColor,
                        "Number of CPU usage data points available for export");
        
        renderMetricCard("Memory History", 
                        std::to_string((int)data.memoryHistory.size()).c_str(), 
                        "points", 
                        g_colorScheme.memoryColor,
                        "Number of memory usage data points available for export");
        
        renderMetricCard("Temperature History", 
                        std::to_string((int)data.temperatureHistory.size()).c_str(), 
                        "points", 
                        g_colorScheme.temperatureColor,
                        "Number of temperature data points available for export");
        
        renderMetricCard("Network History", 
                        std::to_string((int)data.networkRxHistory.size()).c_str(), 
                        "points", 
                        g_colorScheme.networkRxColor,
                        "Number of network data points available for export");
    }
    
    nextResponsiveColumn();
    
    // Right column - Export Controls
    {
        ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textSecondary);
        ImGui::Text("Export Settings");
        ImGui::PopStyleColor();
        ImGui::Separator();
        
        // Export format selection with enhanced styling
        static int exportFormat = 0;
        ImGui::Text("Export Format:");
        
        ImGui::PushStyleColor(ImGuiCol_Button, g_colorScheme.success);
        if (ImGui::RadioButton("📊 CSV (Spreadsheet)", &exportFormat, 0)) {}
        ImGui::PopStyleColor();
        ImGui::SameLine(); renderHelpMarker("Export data in CSV format for Excel, LibreOffice, or other spreadsheet applications");
        
        ImGui::PushStyleColor(ImGuiCol_Button, g_colorScheme.info);
        if (ImGui::RadioButton("🔧 JSON (Technical)", &exportFormat, 1)) {}
        ImGui::PopStyleColor();
        ImGui::SameLine(); renderHelpMarker("Export data in JSON format for programmatic analysis and processing");
        
        ImGui::Spacing();
        
        // Export filename with better styling
        static char exportFilename[256] = "system_monitor_data";
        ImGui::Text("Filename:");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##filename", exportFilename, sizeof(exportFilename));
        
        ImGui::Spacing();
        
        // Enhanced export buttons
        string format = (exportFormat == 0) ? "csv" : "json";
        string extension = (exportFormat == 0) ? ".csv" : ".json";
        string fullFilename = data.config.exportPath + exportFilename + extension;
        
        ImGui::PushStyleColor(ImGuiCol_Button, g_colorScheme.primary);
        if (ImGui::Button("📤 Export Data", ImVec2(-1, 40))) {
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
        ImGui::PopStyleColor();
        
        ImGui::PushStyleColor(ImGuiCol_Button, g_colorScheme.warning);
        if (ImGui::Button("📁 Open Export Directory", ImVec2(-1, 30))) {
            string openCmd = "xdg-open " + data.config.exportPath;
            system(openCmd.c_str());
        }
        ImGui::PopStyleColor();
        
        // Enhanced export result popups
        if (ImGui::BeginPopupModal("Export Success", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.success);
            ImGui::Text("✅ Data exported successfully!");
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::Text("📍 Location: %s", fullFilename.c_str());
            ImGui::Text("📊 Format: %s", format.c_str());
            if (ImGui::Button("OK", ImVec2(120, 30))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        if (ImGui::BeginPopupModal("Export Failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.danger);
            ImGui::Text("❌ Export failed!");
            ImGui::PopStyleColor();
            ImGui::Separator();
            ImGui::Text("Please check the export path and permissions.");
            ImGui::Text("Path: %s", data.config.exportPath.c_str());
            if (ImGui::Button("OK", ImVec2(120, 30))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    
    endResponsiveLayout();
}

void renderHistoricalGraphs(const HistoricalData& data)
{
    renderSectionHeader("Historical Data Visualization", "📈");
    
    // Enhanced graphs with better colors and tooltips
    beginResponsiveLayout(2);
    
    // CPU History Graph
    if (!data.cpuHistory.empty()) {
        vector<float> cpuValues;
        for (const auto& point : data.cpuHistory) {
            cpuValues.push_back(point.value);
        }
        
        renderEnhancedGraph("🖥 CPU Usage History", cpuValues, g_colorScheme.cpuColor, 0.0f, 100.0f,
                           "CPU usage percentage over time. Shows processing load and performance patterns.");
    }
    
    nextResponsiveColumn();
    
    // Memory History Graph
    if (!data.memoryHistory.empty()) {
        vector<float> memValues;
        for (const auto& point : data.memoryHistory) {
            memValues.push_back(point.value);
        }
        
        renderEnhancedGraph("🧠 Memory Usage History", memValues, g_colorScheme.memoryColor, 0.0f, 100.0f,
                           "Memory usage percentage over time. Helps identify memory leaks and usage patterns.");
    }
    
    nextResponsiveColumn();
    
    // Temperature History Graph
    if (!data.temperatureHistory.empty()) {
        vector<float> tempValues;
        for (const auto& point : data.temperatureHistory) {
            tempValues.push_back(point.value);
        }
        
        renderEnhancedGraph("🌡 Temperature History", tempValues, g_colorScheme.temperatureColor, 20.0f, 100.0f,
                           "System temperature over time. Monitor thermal performance and cooling efficiency.");
    }
    
    nextResponsiveColumn();
    
    // Network History Graph (Combined RX/TX)
    if (!data.networkRxHistory.empty() && !data.networkTxHistory.empty()) {
        vector<float> netValues;
        for (size_t i = 0; i < std::min(data.networkRxHistory.size(), data.networkTxHistory.size()); ++i) {
            float totalSpeed = (data.networkRxHistory[i].value + data.networkTxHistory[i].value) / (1024 * 1024); // Convert to MB/s
            netValues.push_back(totalSpeed);
        }
        
        if (!netValues.empty()) {
            float maxSpeed = *std::max_element(netValues.begin(), netValues.end());
            renderEnhancedGraph("🌐 Network Activity History", netValues, g_colorScheme.networkRxColor, 0.0f, maxSpeed * 1.1f,
                               "Combined network activity (RX + TX) in MB/s. Monitor bandwidth usage patterns.");
        }
    }
    
    endResponsiveLayout();
}

void renderAdvancedSystemOverview(const HistoricalData& data, const CPUMonitor& cpu,
                                const MemoryProcessMonitor& memory, const ThermalMonitor& thermal)
{
    renderSectionHeader("Advanced System Overview", "🎛");
    
    // System health status with enhanced visuals
    bool anyAlerts = data.config.cpuAlert.isActive || 
                    data.config.memoryAlert.isActive || 
                    data.config.temperatureAlert.isActive;
    
    // Main health indicator
    ImGui::PushStyleColor(ImGuiCol_ChildBg, anyAlerts ? 
        ImVec4(0.2f, 0.1f, 0.1f, 1.0f) : ImVec4(0.1f, 0.2f, 0.1f, 1.0f));
    
    if (ImGui::BeginChild("HealthStatus", ImVec2(0, 80), true)) {
        ImVec4 healthColor = anyAlerts ? g_colorScheme.danger : g_colorScheme.success;
        string healthIcon = anyAlerts ? "🚨" : "✅";
        string healthText = anyAlerts ? "SYSTEM ALERTS ACTIVE" : "SYSTEM OPERATING NORMALLY";
        
        ImGui::PushStyleColor(ImGuiCol_Text, healthColor);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
        ImGui::Text("   %s %s", healthIcon.c_str(), healthText.c_str());
        ImGui::PopStyleColor();
        
        if (anyAlerts) {
            ImGui::SameLine();
            renderHelpMarker("One or more system resources have exceeded their alert thresholds. Check the Alerts tab for details.");
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    
    // Enhanced metric cards with trends
    beginResponsiveLayout(4);
    
    // CPU Metric Card
    {
        float cpuAvg = 0.0f, cpuMax = 0.0f;
        if (!data.cpuHistory.empty()) {
            cpuAvg = calculateMovingAverage(data.cpuHistory, data.cpuHistory.size());
            cpuMax = std::max_element(data.cpuHistory.begin(), data.cpuHistory.end(),
                [](const HistoricalDataPoint& a, const HistoricalDataPoint& b) {
                    return a.value < b.value;
                })->value;
        }
        renderEnhancedMetricDisplay("🖥 CPU", cpu.currentCPUPercent, cpuAvg, cpuMax, "%", 
                                   g_colorScheme.cpuColor, data.config.cpuAlert.warningLevel, data.config.cpuAlert.criticalLevel);
    }
    
    nextResponsiveColumn();
    
    // Memory Metric Card
    {
        float memAvg = 0.0f, memMax = 0.0f;
        if (!data.memoryHistory.empty()) {
            memAvg = calculateMovingAverage(data.memoryHistory, data.memoryHistory.size());
            memMax = std::max_element(data.memoryHistory.begin(), data.memoryHistory.end(),
                [](const HistoricalDataPoint& a, const HistoricalDataPoint& b) {
                    return a.value < b.value;
                })->value;
        }
        renderEnhancedMetricDisplay("🧠 Memory", memory.memory.memUsedPercent, memAvg, memMax, "%", 
                                   g_colorScheme.memoryColor, data.config.memoryAlert.warningLevel, data.config.memoryAlert.criticalLevel);
    }
    
    nextResponsiveColumn();
    
    // Temperature Metric Card
    {
        float tempAvg = 0.0f, tempMax = 0.0f;
        if (!data.temperatureHistory.empty()) {
            tempAvg = calculateMovingAverage(data.temperatureHistory, data.temperatureHistory.size());
            tempMax = std::max_element(data.temperatureHistory.begin(), data.temperatureHistory.end(),
                [](const HistoricalDataPoint& a, const HistoricalDataPoint& b) {
                    return a.value < b.value;
                })->value;
        }
        renderEnhancedMetricDisplay("🌡 Temperature", thermal.currentMaxTemp, tempAvg, tempMax, "°C", 
                                   g_colorScheme.temperatureColor, data.config.temperatureAlert.warningLevel, data.config.temperatureAlert.criticalLevel);
    }
    
    nextResponsiveColumn();
    
    // Disk Usage Card
    {
        renderEnhancedMetricDisplay("💾 Disk", memory.disk.usedPercent, memory.disk.usedPercent, memory.disk.usedPercent, "%", 
                                   g_colorScheme.diskColor, 85.0f, 95.0f);
    }
    
    endResponsiveLayout();
    
    // Monitoring statistics
    ImGui::Spacing();
    renderSectionHeader("Monitoring Statistics");
    
    beginResponsiveLayout(3);
    
    auto uptime = std::chrono::steady_clock::now() - data.startTime;
    renderMetricCard("Monitoring Uptime", 
                    formatDuration(uptime).c_str(), 
                    "", 
                    g_colorScheme.success,
                    "How long the enhanced monitoring system has been running");
    
    nextResponsiveColumn();
    
    renderMetricCard("Total Data Points", 
                    std::to_string(data.totalDataPoints).c_str(), 
                    "", 
                    g_colorScheme.primary,
                    "Total number of data points collected across all metrics");
    
    nextResponsiveColumn();
    
    renderMetricCard("Active Alerts", 
                    std::to_string((int)data.notifications.size()).c_str(), 
                    "", 
                    anyAlerts ? g_colorScheme.danger : g_colorScheme.success,
                    "Number of recent alert notifications");
    
    endResponsiveLayout();
    
    // Quick export button for convenience
    if (data.config.enableDataExport) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, g_colorScheme.info);
        if (ImGui::Button("📤 Quick Export (CSV)", ImVec2(200, 35))) {
            auto now = std::time(nullptr);
            std::ostringstream filename;
            filename << data.config.exportPath << "quick_export_" 
                    << std::put_time(std::localtime(&now), "%Y%m%d_%H%M%S") << ".csv";
            exportToCSV(data, filename.str());
        }
        ImGui::PopStyleColor();
        ImGui::SameLine(); renderHelpMarker("Quickly export current system data with automatic timestamp");
    }
}
#include "optimized_header.h"

// Optimized rendering with memory pooling and efficient data handling
void renderOptimizedCPUGraph(OptimizedCPUMonitor& monitor) {
    PERF_TIMER("renderOptimizedCPUGraph");
    
    float current_cpu = monitor.current_cpu_percent.get();
    ImGui::Text("CPU Usage: %.1f%%", current_cpu);
    ImGui::SameLine();
    
    if (ImGui::Button(monitor.is_paused.load() ? "Resume" : "Pause")) {
        monitor.is_paused.store(!monitor.is_paused.load());
    }
    
    float update_rate = monitor.update_rate.load();
    if (ImGui::SliderFloat("Update Rate (FPS)", &update_rate, 1.0f, 120.0f, "%.1f")) {
        monitor.update_rate.store(update_rate);
    }
    
    float y_scale = monitor.y_scale.load();
    if (ImGui::SliderFloat("Y-Scale", &y_scale, 50.0f, 200.0f, "%.1f%%")) {
        monitor.y_scale.store(y_scale);
    }
    
    // Efficient graph rendering - get copy once
    auto cpu_data = monitor.cpu_history.get_copy();
    if (!cpu_data.empty()) {
        // Use raw pointer for ImGui to avoid repeated conversions
        ImGui::PlotLines("CPU Usage", cpu_data.data(), cpu_data.size(), 
                        0, nullptr, 0.0f, y_scale, ImVec2(0, 200));
        
        // Calculate statistics efficiently
        float sum = 0.0f, max_val = 0.0f;
        for (float val : cpu_data) {
            sum += val;
            if (val > max_val) max_val = val;
        }
        float avg = sum / cpu_data.size();
        
        ImGui::Text("Current: %.1f%% | Avg: %.1f%% | Max: %.1f%%", 
                   current_cpu, avg, max_val);
    }
}

void renderOptimizedThermalGraph(OptimizedThermalMonitor& monitor) {
    PERF_TIMER("renderOptimizedThermalGraph");
    
    float current_temp = monitor.current_max_temp.get();
    ImGui::Text("Max Temperature: %.1f°C", current_temp);
    ImGui::SameLine();
    
    if (ImGui::Button(monitor.is_paused.load() ? "Resume" : "Pause")) {
        monitor.is_paused.store(!monitor.is_paused.load());
    }
    
    float update_rate = monitor.update_rate.load();
    if (ImGui::SliderFloat("Update Rate (FPS)", &update_rate, 1.0f, 60.0f, "%.1f")) {
        monitor.update_rate.store(update_rate);
    }
    
    float y_scale = monitor.y_scale.load();
    if (ImGui::SliderFloat("Y-Scale", &y_scale, 50.0f, 150.0f, "%.1f°C")) {
        monitor.y_scale.store(y_scale);
    }
    
    // Display individual sensors
    auto sensors = monitor.sensors.get();
    if (!sensors.empty()) {
        ImGui::Text("Temperature Sensors:");
        for (const auto& sensor : sensors) {
            if (sensor.isValid) {
                ImVec4 color = sensor.temperature > 80.0f ? ImVec4(1, 0, 0, 1) : 
                              sensor.temperature > 60.0f ? ImVec4(1, 1, 0, 1) : 
                              ImVec4(0, 1, 0, 1);
                ImGui::TextColored(color, "%s: %.1f°C", sensor.name.c_str(), sensor.temperature);
            }
        }
    }
    
    // Efficient temperature history graph
    auto temp_data = monitor.temp_history.get_copy();
    if (!temp_data.empty()) {
        ImGui::PlotLines("Temperature", temp_data.data(), temp_data.size(), 
                        0, nullptr, 0.0f, y_scale, ImVec2(0, 200));
        
        float sum = 0.0f, max_val = 0.0f;
        for (float val : temp_data) {
            sum += val;
            if (val > max_val) max_val = val;
        }
        float avg = sum / temp_data.size();
        
        ImGui::Text("Current: %.1f°C | Avg: %.1f°C | Max: %.1f°C", 
                   current_temp, avg, max_val);
    }
}

void renderOptimizedMemoryProcessInterface(OptimizedMemoryProcessMonitor& monitor) {
    PERF_TIMER("renderOptimizedMemoryProcessInterface");
    
    ImGui::Text("Memory and Process Monitoring");
    ImGui::Separator();
    
    // Memory info
    MemoryInfo memory = monitor.memory.get();
    
    ImGui::Text("Memory Usage");
    char memory_overlay[64];
    snprintf(memory_overlay, sizeof(memory_overlay), "%.1f%% (%s / %s)", 
             memory.memUsedPercent,
             formatBytes(memory.memUsed * 1024).c_str(),
             formatBytes(memory.memTotal * 1024).c_str());
    
    ImGui::ProgressBar(memory.memUsedPercent / 100.0f, ImVec2(0.0f, 0.0f), memory_overlay);
    
    if (memory.swapTotal > 0) {
        ImGui::Text("Swap Usage");
        char swap_overlay[64];
        snprintf(swap_overlay, sizeof(swap_overlay), "%.1f%% (%s / %s)", 
                 memory.swapUsedPercent,
                 formatBytes(memory.swapUsed * 1024).c_str(),
                 formatBytes(memory.swapTotal * 1024).c_str());
        
        ImGui::ProgressBar(memory.swapUsedPercent / 100.0f, ImVec2(0.0f, 0.0f), swap_overlay);
    }
    
    ImGui::Spacing();
    
    // Process table with optimized rendering
    auto processes = monitor.processes.get();
    if (!processes.empty()) {
        ImGui::Text("Process List (%zu processes)", processes.size());
        
        // Search filter
        string current_filter = monitor.search_filter.get();
        char filter_buffer[256];
        strncpy(filter_buffer, current_filter.c_str(), sizeof(filter_buffer) - 1);
        filter_buffer[sizeof(filter_buffer) - 1] = '\0';
        
        if (ImGui::InputText("Search", filter_buffer, sizeof(filter_buffer))) {
            monitor.search_filter.update(string(filter_buffer));
        }
        
        // Sort controls
        bool sort_cpu = monitor.sort_by_cpu.load();
        bool sort_mem = monitor.sort_by_memory.load();
        
        if (ImGui::Checkbox("Sort by CPU", &sort_cpu)) {
            monitor.sort_by_cpu.store(sort_cpu);
            if (sort_cpu) monitor.sort_by_memory.store(false);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Sort by Memory", &sort_mem)) {
            monitor.sort_by_memory.store(sort_mem);
            if (sort_mem) monitor.sort_by_cpu.store(false);
        }
        
        // Efficient process table
        if (ImGui::BeginTable("ProcessTable", 6, 
                             ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | 
                             ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable, 
                             ImVec2(0, 300))) {
            
            ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 60);
            ImGui::TableSetupColumn("CPU%", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Mem%", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Memory", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableHeadersRow();
            
            // Sort and filter processes efficiently
            vector<ProcessInfo> display_processes = processes;
            
            // Apply filter
            if (!current_filter.empty()) {
                display_processes.erase(
                    std::remove_if(display_processes.begin(), display_processes.end(),
                        [&current_filter](const ProcessInfo& p) {
                            return p.name.find(current_filter) == string::npos;
                        }),
                    display_processes.end());
            }
            
            // Sort processes
            if (sort_cpu) {
                std::sort(display_processes.begin(), display_processes.end(),
                         [](const ProcessInfo& a, const ProcessInfo& b) {
                             return a.cpuPercent > b.cpuPercent;
                         });
            } else if (sort_mem) {
                std::sort(display_processes.begin(), display_processes.end(),
                         [](const ProcessInfo& a, const ProcessInfo& b) {
                             return a.memPercent > b.memPercent;
                         });
            }
            
            // Render top processes (limit for performance)
            int max_display = std::min(100, static_cast<int>(display_processes.size()));
            for (int i = 0; i < max_display; i++) {
                const auto& process = display_processes[i];
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", process.pid);
                
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", process.name.c_str());
                
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%c", process.state);
                
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%.1f", process.cpuPercent);
                
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%.1f", process.memPercent);
                
                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%s", formatBytes(process.rss * 1024).c_str());
            }
            
            ImGui::EndTable();
        }
    }
}

void renderOptimizedNetworkInterface(OptimizedNetworkMonitor& monitor) {
    PERF_TIMER("renderOptimizedNetworkInterface");
    
    if (ImGui::BeginTabBar("NetworkTabs")) {
        // Overview tab
        if (ImGui::BeginTabItem("Overview")) {
            ImGui::Text("Network Interface Overview");
            ImGui::Separator();
            
            if (ImGui::Button(monitor.is_paused.load() ? "Resume" : "Pause")) {
                monitor.is_paused.store(!monitor.is_paused.load());
            }
            ImGui::SameLine();
            
            float update_rate = monitor.update_rate.load();
            if (ImGui::SliderFloat("Update Rate", &update_rate, 0.5f, 10.0f, "%.1f Hz")) {
                monitor.update_rate.store(update_rate);
            }
            
            auto interfaces = monitor.interfaces.get();
            if (!interfaces.empty()) {
                if (ImGui::BeginTable("InterfaceTable", 6, 
                                     ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    
                    ImGui::TableSetupColumn("Interface");
                    ImGui::TableSetupColumn("State");
                    ImGui::TableSetupColumn("RX Speed");
                    ImGui::TableSetupColumn("TX Speed");
                    ImGui::TableSetupColumn("RX Total");
                    ImGui::TableSetupColumn("TX Total");
                    ImGui::TableHeadersRow();
                    
                    for (const auto& iface : interfaces) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%s", iface.name.c_str());
                        
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%s", iface.state.c_str());
                        
                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%s", formatNetworkSpeed(iface.rxSpeed).c_str());
                        
                        ImGui::TableSetColumnIndex(3);
                        ImGui::Text("%s", formatNetworkSpeed(iface.txSpeed).c_str());
                        
                        ImGui::TableSetColumnIndex(4);
                        ImGui::Text("%s", formatNetworkBytes(iface.rxBytes).c_str());
                        
                        ImGui::TableSetColumnIndex(5);
                        ImGui::Text("%s", formatNetworkBytes(iface.txBytes).c_str());
                    }
                    
                    ImGui::EndTable();
                }
            }
            
            ImGui::EndTabItem();
        }
        
        // RX visualization
        if (ImGui::BeginTabItem("RX (Download)")) {
            auto rx_data = monitor.rx_speed_history.get_copy();
            if (!rx_data.empty()) {
                float current_rx = rx_data.back();
                ImGui::Text("Current RX Speed: %s", formatNetworkSpeed(current_rx).c_str());
                
                float max_rx = monitor.max_rx_speed.load();
                float scale = std::max(max_rx * 1.1f, 1024.0f);
                
                ImGui::PlotLines("RX Speed", rx_data.data(), rx_data.size(), 
                               0, nullptr, 0.0f, scale, ImVec2(0, 200));
                
                float sum = 0.0f, max_val = 0.0f;
                for (float val : rx_data) {
                    sum += val;
                    if (val > max_val) max_val = val;
                }
                float avg = sum / rx_data.size();
                
                ImGui::Text("Average: %s | Peak: %s", 
                           formatNetworkSpeed(avg).c_str(),
                           formatNetworkSpeed(max_val).c_str());
            }
            
            ImGui::EndTabItem();
        }
        
        // TX visualization
        if (ImGui::BeginTabItem("TX (Upload)")) {
            auto tx_data = monitor.tx_speed_history.get_copy();
            if (!tx_data.empty()) {
                float current_tx = tx_data.back();
                ImGui::Text("Current TX Speed: %s", formatNetworkSpeed(current_tx).c_str());
                
                float max_tx = monitor.max_tx_speed.load();
                float scale = std::max(max_tx * 1.1f, 1024.0f);
                
                ImGui::PlotLines("TX Speed", tx_data.data(), tx_data.size(), 
                               0, nullptr, 0.0f, scale, ImVec2(0, 200));
                
                float sum = 0.0f, max_val = 0.0f;
                for (float val : tx_data) {
                    sum += val;
                    if (val > max_val) max_val = val;
                }
                float avg = sum / tx_data.size();
                
                ImGui::Text("Average: %s | Peak: %s", 
                           formatNetworkSpeed(avg).c_str(),
                           formatNetworkSpeed(max_val).c_str());
            }
            
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
}
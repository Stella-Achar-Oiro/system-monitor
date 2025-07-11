#include "header.h"
#include <algorithm>

// Global UI state instances
UIColorScheme g_colorScheme;
UILayout g_layout;

// ===== UI THEME AND STYLING =====

void setupUITheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Enhanced rounded corners and spacing
    style.WindowRounding = 8.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;
    
    // Improved padding and spacing
    style.WindowPadding = ImVec2(12, 12);
    style.FramePadding = ImVec2(10, 6);
    style.CellPadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(10, 6);
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.TouchExtraPadding = ImVec2(0, 0);
    style.IndentSpacing = 25;
    style.ScrollbarSize = 14;
    style.GrabMinSize = 12;
    
    // Enhanced borders
    style.WindowBorderSize = 1;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.FrameBorderSize = 0;
    style.TabBorderSize = 0;
    
    // Apply our custom color scheme
    setupUIColors(g_colorScheme);
}

void setupUIColors(const UIColorScheme& colors) {
    ImVec4* styleColors = ImGui::GetStyle().Colors;
    
    // Window colors
    styleColors[ImGuiCol_WindowBg] = colors.background;
    styleColors[ImGuiCol_ChildBg] = colors.backgroundSecondary;
    styleColors[ImGuiCol_PopupBg] = colors.background;
    
    // Text colors
    styleColors[ImGuiCol_Text] = colors.textPrimary;
    styleColors[ImGuiCol_TextDisabled] = colors.textMuted;
    
    // Frame colors
    styleColors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    styleColors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    styleColors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    
    // Title colors
    styleColors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    styleColors[ImGuiCol_TitleBgActive] = colors.primaryDark;
    styleColors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.08f, 0.08f, 0.75f);
    
    // Menu colors
    styleColors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    
    // Button colors
    styleColors[ImGuiCol_Button] = colors.primary;
    styleColors[ImGuiCol_ButtonHovered] = colors.primaryLight;
    styleColors[ImGuiCol_ButtonActive] = colors.primaryDark;
    
    // Header colors
    styleColors[ImGuiCol_Header] = ImVec4(colors.primary.x, colors.primary.y, colors.primary.z, 0.31f);
    styleColors[ImGuiCol_HeaderHovered] = ImVec4(colors.primary.x, colors.primary.y, colors.primary.z, 0.80f);
    styleColors[ImGuiCol_HeaderActive] = colors.primary;
    
    // Tab colors
    styleColors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    styleColors[ImGuiCol_TabHovered] = colors.primaryLight;
    styleColors[ImGuiCol_TabActive] = colors.primary;
    styleColors[ImGuiCol_TabUnfocused] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    styleColors[ImGuiCol_TabUnfocusedActive] = colors.primaryDark;
    
    // Plot colors
    styleColors[ImGuiCol_PlotLines] = colors.primary;
    styleColors[ImGuiCol_PlotLinesHovered] = colors.primaryLight;
    styleColors[ImGuiCol_PlotHistogram] = colors.primary;
    styleColors[ImGuiCol_PlotHistogramHovered] = colors.primaryLight;
    
    // Borders
    styleColors[ImGuiCol_Border] = colors.border;
    styleColors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    
    // Scrollbar
    styleColors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    styleColors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    styleColors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    styleColors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    
    // Check mark
    styleColors[ImGuiCol_CheckMark] = colors.primary;
    
    // Slider
    styleColors[ImGuiCol_SliderGrab] = colors.primary;
    styleColors[ImGuiCol_SliderGrabActive] = colors.primaryLight;
    
    // Separator
    styleColors[ImGuiCol_Separator] = colors.border;
    styleColors[ImGuiCol_SeparatorHovered] = colors.primary;
    styleColors[ImGuiCol_SeparatorActive] = colors.primaryLight;
    
    // Resize grip
    styleColors[ImGuiCol_ResizeGrip] = ImVec4(colors.primary.x, colors.primary.y, colors.primary.z, 0.20f);
    styleColors[ImGuiCol_ResizeGripHovered] = ImVec4(colors.primary.x, colors.primary.y, colors.primary.z, 0.67f);
    styleColors[ImGuiCol_ResizeGripActive] = ImVec4(colors.primary.x, colors.primary.y, colors.primary.z, 0.95f);
    
    // Table colors
    styleColors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    styleColors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    styleColors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    styleColors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    styleColors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
}

// ===== COLOR UTILITY FUNCTIONS =====

ImVec4 getResourceColor(float value, float warningThreshold, float criticalThreshold) {
    if (value >= criticalThreshold) {
        return g_colorScheme.danger;
    } else if (value >= warningThreshold) {
        return g_colorScheme.warning;
    } else {
        return g_colorScheme.success;
    }
}

ImVec4 getTrendColor(float trend) {
    if (trend > 0.5f) {
        return g_colorScheme.danger;
    } else if (trend > 0.1f) {
        return g_colorScheme.warning;
    } else if (trend < -0.1f) {
        return g_colorScheme.success;
    } else {
        return g_colorScheme.textSecondary;
    }
}

// ===== ENHANCED UI COMPONENTS =====

void renderSectionHeader(const char* title, const char* icon) {
    ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textPrimary);
    ImGui::PushFont(nullptr); // Use default bold font if available
    
    if (icon) {
        ImGui::Text("%s %s", icon, title);
    } else {
        ImGui::Text("%s", title);
    }
    
    ImGui::PopFont();
    ImGui::PopStyleColor();
    
    // Add a subtle separator
    ImGui::PushStyleColor(ImGuiCol_Separator, g_colorScheme.primary);
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();
}

void renderMetricCard(const char* title, const char* value, const char* unit, ImVec4 color, const char* tooltip) {
    // Card background
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.14f, 0.14f, 0.14f, 1.00f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    
    if (ImGui::BeginChild(title, ImVec2(0, 80), true, ImGuiWindowFlags_NoScrollbar)) {
        ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textSecondary);
        ImGui::Text("%s", title);
        ImGui::PopStyleColor();
        
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::PushFont(nullptr); // Large font if available
        if (unit) {
            ImGui::Text("%s %s", value, unit);
        } else {
            ImGui::Text("%s", value);
        }
        ImGui::PopFont();
        ImGui::PopStyleColor();
        
        if (tooltip && ImGui::IsItemHovered()) {
            renderTooltip(tooltip);
        }
    }
    ImGui::EndChild();
    
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void renderProgressBar(const char* label, float value, float max_value, ImVec4 color, const char* tooltip) {
    float fraction = max_value > 0 ? value / max_value : 0.0f;
    fraction = std::clamp(fraction, 0.0f, 1.0f);
    
    // Custom progress bar with color
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
    ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f), "");
    ImGui::PopStyleColor();
    
    // Label overlay
    ImVec2 progressBarMin = ImGui::GetItemRectMin();
    ImVec2 progressBarMax = ImGui::GetItemRectMax();
    ImVec2 labelSize = ImGui::CalcTextSize(label);
    ImVec2 labelPos = ImVec2(
        progressBarMin.x + (progressBarMax.x - progressBarMin.x - labelSize.x) * 0.5f,
        progressBarMin.y + (progressBarMax.y - progressBarMin.y - labelSize.y) * 0.5f
    );
    
    ImGui::GetWindowDrawList()->AddText(labelPos, IM_COL32(255, 255, 255, 255), label);
    
    if (tooltip && ImGui::IsItemHovered()) {
        renderTooltip(tooltip);
    }
}

void renderEnhancedGraph(const char* title, const vector<float>& data, ImVec4 color, float minValue, float maxValue, const char* tooltip) {
    if (data.empty()) return;
    
    ImGui::PushStyleColor(ImGuiCol_PlotLines, color);
    ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, ImVec4(color.x + 0.2f, color.y + 0.2f, color.z + 0.2f, 1.0f));
    
    ImVec2 graphSize = ImVec2(0, g_layout.graphHeight);
    
    // Create a more sophisticated graph with grid lines
    ImGui::PlotLines(title, data.data(), data.size(), 0, nullptr, minValue, maxValue, graphSize);
    
    ImGui::PopStyleColor(2);
    
    // Add statistics overlay
    if (!data.empty()) {
        float current = data.back();
        float average = std::accumulate(data.begin(), data.end(), 0.0f) / data.size();
        float maximum = *std::max_element(data.begin(), data.end());
        
        ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textMuted);
        ImGui::Text("Current: %.1f | Avg: %.1f | Max: %.1f", current, average, maximum);
        ImGui::PopStyleColor();
    }
    
    if (tooltip && ImGui::IsItemHovered()) {
        renderTooltip(tooltip);
    }
}

void renderStatusBadge(const char* text, ImVec4 color) {
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    
    ImGui::SmallButton(text);
    
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
}

void renderTooltip(const char* text) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(text);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

void renderHelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        renderTooltip(desc);
    }
}

// ===== RESPONSIVE LAYOUT SYSTEM =====

void beginResponsiveLayout(int columns) {
    // Update layout based on current window size
    g_layout.updateLayout(ImGui::GetWindowSize());
    
    // Use adaptive column count
    int adaptiveColumns = g_layout.isCompact ? 1 : columns;
    
    ImGui::Columns(adaptiveColumns, nullptr, true);
}

void nextResponsiveColumn() {
    if (!g_layout.isCompact) {
        ImGui::NextColumn();
    } else {
        ImGui::Spacing();
    }
}

void endResponsiveLayout() {
    ImGui::Columns(1);
}

// ===== ENHANCED METRIC DISPLAY =====

void renderEnhancedMetricDisplay(const char* title, float currentValue, float averageValue, 
                                float maxValue, const char* unit, ImVec4 color,
                                float warningThreshold, float criticalThreshold) {
    
    // Card container
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.14f, 0.14f, 0.14f, 1.00f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
    
    float cardHeight = g_layout.isCompact ? 120 : 150;
    if (ImGui::BeginChild((string(title) + "_card").c_str(), ImVec2(0, cardHeight), true, ImGuiWindowFlags_NoScrollbar)) {
        
        // Header with status
        ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textSecondary);
        ImGui::Text("%s", title);
        ImGui::PopStyleColor();
        
        ImGui::SameLine();
        ImVec4 statusColor = getResourceColor(currentValue, warningThreshold, criticalThreshold);
        ImGui::PushStyleColor(ImGuiCol_Text, statusColor);
        if (currentValue >= criticalThreshold) {
            ImGui::Text("CRITICAL");
        } else if (currentValue >= warningThreshold) {
            ImGui::Text("WARNING");
        } else {
            ImGui::Text("NORMAL");
        }
        ImGui::PopStyleColor();
        
        ImGui::Spacing();
        
        // Large current value
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Text("%.1f%s", currentValue, unit);
        ImGui::PopStyleColor();
        
        // Progress bar
        renderProgressBar("", currentValue, 100.0f, statusColor, nullptr);
        
        // Statistics
        ImGui::PushStyleColor(ImGuiCol_Text, g_colorScheme.textMuted);
        ImGui::Text("Avg: %.1f%s | Max: %.1f%s", averageValue, unit, maxValue, unit);
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}
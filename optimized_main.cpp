#include "optimized_header.h"
#include <SDL.h>

// About Desktop OpenGL function loaders:
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
#include <glad/gl.h>
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
#include <glbinding/Binding.h>
#include <glbinding/gl/gl.h>
using namespace gl;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
#include <glbinding/glbinding.h>
#include <glbinding/gl/gl.h>
using namespace gl;
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// Performance and error recovery globals
static bool g_thermal_initialized = false;
static std::chrono::steady_clock::time_point g_last_performance_print;

// Error recovery and edge case handling
void handleCriticalError(const std::string& component, const std::string& error) {
    g_error_handler.log_error(component, error, ErrorHandler::ErrorLevel::CRITICAL);
    
    // Try to recover based on component
    if (component == "Data Collection") {
        // Restart data collection threads
        if (g_data_collection_manager.is_running()) {
            g_data_collection_manager.stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            g_data_collection_manager.start();
        }
    } else if (component == "File Reading") {
        // Clear caches and retry
        g_proc_reader.clear_cache();
    }
}

void initializeOptimizedMonitoring() {
    try {
        // Initialize thermal monitoring
        if (!g_thermal_initialized) {
            auto sensors = discoverThermalSensorsOptimized();
            g_optimized_thermal_monitor.sensors.update(sensors);
            g_thermal_initialized = true;
        }
        
        // Set initial history sizes based on available memory
        struct sysinfo sys_info;
        if (sysinfo(&sys_info) == 0) {
            // Adjust history sizes based on available memory
            size_t available_mb = sys_info.freeram / (1024 * 1024);
            size_t history_size = available_mb > 4096 ? 500 : 200; // More history if lots of RAM
            
            g_optimized_cpu_monitor.cpu_history.set_max_size(history_size);
            g_optimized_thermal_monitor.temp_history.set_max_size(history_size);
            g_optimized_network_monitor.rx_speed_history.set_max_size(history_size);
            g_optimized_network_monitor.tx_speed_history.set_max_size(history_size);
        }
        
        // Start data collection threads
        g_data_collection_manager.start();
        
    } catch (const std::exception& e) {
        handleCriticalError("Initialization", e.what());
    }
}

void renderOptimizedSystemOverview() {
    PERF_TIMER("renderOptimizedSystemOverview");
    
    // System info in columns
    ImGui::Columns(2, "SystemColumns", true);
    
    // Left column - System Information
    ImGui::Text("System Information");
    ImGui::Separator();
    ImGui::Text("OS: %s", getOsName());
    ImGui::Text("Current User: %s", getCurrentUser().c_str());
    ImGui::Text("Hostname: %s", getHostname().c_str());
    ImGui::Text("Total Processes: %d", getTotalProcesses());
    ImGui::Text("CPU Model: %s", getCPUModel().c_str());
    
    ImGui::NextColumn();
    
    // Right column - Quick stats
    ImGui::Text("Quick Statistics");
    ImGui::Separator();
    
    float cpu_percent = g_optimized_cpu_monitor.current_cpu_percent.get();
    ImGui::Text("CPU Usage: %.1f%%", cpu_percent);
    
    float max_temp = g_optimized_thermal_monitor.current_max_temp.get();
    ImGui::Text("Max Temperature: %.1f°C", max_temp);
    
    MemoryInfo memory = g_optimized_memory_process_monitor.memory.get();
    ImGui::Text("Memory Usage: %.1f%%", memory.memUsedPercent);
    ImGui::Text("Available Memory: %s", formatBytes(memory.memAvailable * 1024).c_str());
    
    // Network summary
    auto interfaces = g_optimized_network_monitor.interfaces.get();
    float total_rx_speed = 0.0f, total_tx_speed = 0.0f;
    for (const auto& iface : interfaces) {
        if (iface.name != "lo") {
            total_rx_speed += iface.rxSpeed;
            total_tx_speed += iface.txSpeed;
        }
    }
    ImGui::Text("Network: ↓ %s | ↑ %s", 
               formatNetworkSpeed(total_rx_speed).c_str(),
               formatNetworkSpeed(total_tx_speed).c_str());
    
    ImGui::Columns(1);
}

void renderPerformanceStats() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - g_last_performance_print).count();
    
    if (elapsed > 5.0f) { // Print every 5 seconds
        ImGui::Text("Performance Monitor");
        if (ImGui::Button("Print Performance Stats")) {
            g_perf_monitor.print_stats();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Stats")) {
            g_perf_monitor.reset();
        }
        
        g_last_performance_print = now;
    }
    
    // Show error summary
    if (ImGui::Button("Show Error Summary")) {
        g_error_handler.print_error_summary();
    }
}

// Main optimized application
int main(int, char **) {
    // Setup SDL with error handling
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        handleCriticalError("SDL", SDL_GetError());
        return -1;
    }

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Optimized System Monitor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    if (!window) {
        handleCriticalError("SDL", "Failed to create window");
        SDL_Quit();
        return -1;
    }
    
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        handleCriticalError("SDL", "Failed to create OpenGL context");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
    bool err = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress) == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
    bool err = false;
    glbinding::Binding::initialize();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
    bool err = false;
    glbinding::initialize([](const char* name) { return (glbinding::ProcAddress)SDL_GL_GetProcAddress(name); });
#else
    bool err = false;
#endif
    if (err) {
        handleCriticalError("OpenGL", "Failed to initialize OpenGL loader");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style with consistent theming
    ImGui::StyleColorsDark();
    
    // Customize style for better performance and consistency
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.WindowPadding = ImVec2(8, 8);
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 4);
    style.ItemInnerSpacing = ImVec2(4, 4);
    
    // Optimized color scheme
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Initialize optimized monitoring
    initializeOptimizedMonitoring();

    // Background color
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Performance monitoring
    g_last_performance_print = std::chrono::steady_clock::now();

    // Main loop
    bool done = false;
    while (!done) {
        PERF_TIMER("MainLoop");
        
        // Poll and handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        // Create optimized full-screen interface
        {
            ImVec2 mainDisplay = io.DisplaySize;
            
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(mainDisplay);
            
            ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoResize | 
                                          ImGuiWindowFlags_NoMove | 
                                          ImGuiWindowFlags_NoCollapse |
                                          ImGuiWindowFlags_MenuBar;
            
            if (ImGui::Begin("Optimized System Monitor", nullptr, windowFlags)) {
                // Menu bar with performance info
                if (ImGui::BeginMenuBar()) {
                    if (ImGui::BeginMenu("View")) {
                        ImGui::MenuItem("Performance Monitor", nullptr, false, false);
                        ImGui::Separator();
                        ImGui::MenuItem("Dark Theme", nullptr, true);
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Performance")) {
                        renderPerformanceStats();
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenuBar();
                }
                
                // Main tabbed interface with optimized rendering
                if (ImGui::BeginTabBar("MainTabs", ImGuiTabBarFlags_None)) {
                    // System Overview Tab
                    if (ImGui::BeginTabItem("System Overview")) {
                        renderOptimizedSystemOverview();
                        ImGui::EndTabItem();
                    }
                    
                    // CPU & Thermal Tab
                    if (ImGui::BeginTabItem("CPU & Thermal")) {
                        if (ImGui::BeginTabBar("CPUThermalTabs")) {
                            if (ImGui::BeginTabItem("CPU")) {
                                renderOptimizedCPUGraph(g_optimized_cpu_monitor);
                                ImGui::EndTabItem();
                            }
                            
                            if (ImGui::BeginTabItem("Thermal")) {
                                renderOptimizedThermalGraph(g_optimized_thermal_monitor);
                                ImGui::EndTabItem();
                            }
                            
                            ImGui::EndTabBar();
                        }
                        ImGui::EndTabItem();
                    }
                    
                    // Memory & Processes Tab
                    if (ImGui::BeginTabItem("Memory & Processes")) {
                        renderOptimizedMemoryProcessInterface(g_optimized_memory_process_monitor);
                        ImGui::EndTabItem();
                    }
                    
                    // Network Tab
                    if (ImGui::BeginTabItem("Network")) {
                        renderOptimizedNetworkInterface(g_optimized_network_monitor);
                        ImGui::EndTabItem();
                    }
                    
                    ImGui::EndTabBar();
                }
                
            }
            ImGui::End();
        }

        // Rendering
        {
            PERF_TIMER("Rendering");
            ImGui::Render();
            glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(window);
        }
    }

    // Cleanup
    g_data_collection_manager.stop();
    g_perf_monitor.print_stats();
    g_error_handler.print_error_summary();
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
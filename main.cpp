#include "header.h"
#include <SDL.h>

/*
NOTE : You are free to change the code as you wish, the main objective is to make the
       application work and pass the audit.

       It will be provided the main function with the following functions :

       - `void systemWindow(const char *id, ImVec2 size, ImVec2 position)`
            This function will draw the system window on your screen
       - `void memoryProcessesWindow(const char *id, ImVec2 size, ImVec2 position)`
            This function will draw the memory and processes window on your screen
       - `void networkWindow(const char *id, ImVec2 size, ImVec2 position)`
            This function will draw the network window on your screen
*/

// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h> // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h> // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h> // Initialize with gladLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
#include <glad/gl.h> // Initialize with gladLoadGL(...) or gladLoaderLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
#define GLFW_INCLUDE_NONE      // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/Binding.h> // Initialize with glbinding::Binding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
#define GLFW_INCLUDE_NONE        // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/glbinding.h> // Initialize with glbinding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// Global monitoring instances
static CPUMonitor g_cpuMonitor;
static ThermalMonitor g_thermalMonitor;
static MemoryProcessMonitor g_memoryProcessMonitor;
static NetworkMonitor g_networkMonitor;
static bool g_thermalInitialized = false;
static bool g_enhancedInitialized = false;

// Performance optimization - track active tabs
static int g_activeMainTab = 0;
static std::chrono::steady_clock::time_point g_lastUIUpdate = std::chrono::steady_clock::now();


// Main code
int main(int, char **)
{
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window *window = SDL_CreateWindow("System Monitor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    if (!window) {
        fprintf(stderr, "Failed to create SDL window: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        fprintf(stderr, "Failed to create OpenGL context: %s\n", SDL_GetError());
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
    bool err = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress) == 0; // glad2 recommend using the windowing library loader instead of the (optionally) bundled one.
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
    bool err = false;
    glbinding::Binding::initialize();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
    bool err = false;
    glbinding::initialize([](const char *name) { return (glbinding::ProcAddress)SDL_GL_GetProcAddress(name); });
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // render bindings
    ImGuiIO &io = ImGui::GetIO();

    // Setup enhanced UI theme
    setupUITheme();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // background color
    // note : you are free to change the style of the application
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
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

        {
            // Create a single fullscreen window with tabbed interface
            ImVec2 mainDisplay = io.DisplaySize;
            g_layout.updateLayout(mainDisplay);
            
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(mainDisplay);
            
            ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoResize | 
                                          ImGuiWindowFlags_NoMove | 
                                          ImGuiWindowFlags_NoCollapse |
                                          ImGuiWindowFlags_MenuBar;
            
            if (ImGui::Begin("System Monitor", nullptr, windowFlags)) {
                // Menu bar
                if (ImGui::BeginMenuBar()) {
                    if (ImGui::BeginMenu("View")) {
                        ImGui::MenuItem("Always on Top", nullptr, false, false);
                        ImGui::Separator();
                        ImGui::MenuItem("Dark Theme", nullptr, true);
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Help")) {
                        ImGui::MenuItem("About", nullptr, false, false);
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenuBar();
                }
                
                // Initialize thermal monitor on first run
                if (!g_thermalInitialized) {
                    initThermalMonitor(g_thermalMonitor);
                    g_thermalInitialized = true;
                }
                
                // Initialize enhanced monitoring on first run
                if (!g_enhancedInitialized) {
                    initializeHistoricalData(getHistoricalData());
                    g_enhancedInitialized = true;
                }
                
                // Update monitors at reduced rate to maintain performance
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration<float>(now - g_lastUIUpdate).count();
                
                // Update basic monitors every 100ms for responsiveness
                if (elapsed >= 0.1f) {
                    updateCPUMonitor(g_cpuMonitor);
                    updateThermalMonitor(g_thermalMonitor);
                    
                    // Update enhanced monitoring data
                    updateMemoryProcessMonitor(g_memoryProcessMonitor);
                    updateNetworkMonitor(g_networkMonitor);
                    updateHistoricalData(getHistoricalData(), g_cpuMonitor, 
                                       g_memoryProcessMonitor, g_thermalMonitor, g_networkMonitor);
                    
                    g_lastUIUpdate = now;
                }
                
                // Main tabbed interface
                if (ImGui::BeginTabBar("MainTabs", ImGuiTabBarFlags_None)) {
                    // System Overview Tab
                    if (ImGui::BeginTabItem("System Overview")) {
                        g_activeMainTab = 0;
                        
                        // Update memory info for overview
                        updateMemoryProcessMonitor(g_memoryProcessMonitor);
                        
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
                        ImGui::Text("CPU Usage: %.1f%%", g_cpuMonitor.currentCPUPercent);
                        ImGui::Text("Max Temperature: %.1f°C", g_thermalMonitor.currentMaxTemp);
                        ImGui::Text("Memory Usage: %.1f%%", g_memoryProcessMonitor.memory.memUsedPercent);
                        ImGui::Text("Available Memory: %s", formatBytes(g_memoryProcessMonitor.memory.memAvailable * 1024).c_str());
                        
                        // Network summary
                        updateNetworkMonitor(g_networkMonitor);
                        float totalRxSpeed = 0.0f, totalTxSpeed = 0.0f;
                        for (const auto& iface : g_networkMonitor.interfaces) {
                            if (iface.name != "lo") {
                                totalRxSpeed += iface.rxSpeed;
                                totalTxSpeed += iface.txSpeed;
                            }
                        }
                        ImGui::Text("Network: ↓ %s | ↑ %s", 
                                   formatNetworkSpeed(totalRxSpeed).c_str(),
                                   formatNetworkSpeed(totalTxSpeed).c_str());
                        
                        ImGui::Columns(1);
                        ImGui::EndTabItem();
                    }
                    
                    // CPU & Thermal Tab
                    if (ImGui::BeginTabItem("CPU & Thermal")) {
                        g_activeMainTab = 1;
                        
                        // Create sub-tabs for CPU and Thermal
                        if (ImGui::BeginTabBar("CPUThermalTabs")) {
                            // CPU tab
                            if (ImGui::BeginTabItem("CPU")) {
                                // Controls
                                ImGui::Text("CPU Usage: %.1f%%", g_cpuMonitor.currentCPUPercent);
                                ImGui::SameLine();
                                
                                // Play/Pause button
                                if (ImGui::Button(g_cpuMonitor.isPaused ? "Resume" : "Pause")) {
                                    g_cpuMonitor.isPaused = !g_cpuMonitor.isPaused;
                                }
                                
                                // FPS slider
                                ImGui::SliderFloat("Update Rate (FPS)", &g_cpuMonitor.updateRate, 1.0f, 120.0f, "%.1f");
                                
                                // Y-scale slider
                                ImGui::SliderFloat("Y-Scale", &g_cpuMonitor.yScale, 50.0f, 200.0f, "%.1f%%");
                                
                                // Graph
                                if (!g_cpuMonitor.cpuHistory.empty()) {
                                    // Convert deque to vector for ImGui
                                    vector<float> plotData(g_cpuMonitor.cpuHistory.begin(), g_cpuMonitor.cpuHistory.end());
                                    
                                    ImGui::PlotLines("CPU Usage", plotData.data(), plotData.size(), 
                                                   0, nullptr, 0.0f, g_cpuMonitor.yScale, ImVec2(0, 200));
                                    
                                    // Current percentage overlay
                                    ImGui::Text("Current: %.1f%% | Avg: %.1f%% | Max: %.1f%%", 
                                               g_cpuMonitor.currentCPUPercent,
                                               // Calculate average
                                               std::accumulate(plotData.begin(), plotData.end(), 0.0f) / plotData.size(),
                                               // Find maximum
                                               *std::max_element(plotData.begin(), plotData.end()));
                                }
                                
                                ImGui::EndTabItem();
                            }
                            
                            // Thermal tab
                            if (ImGui::BeginTabItem("Thermal")) {
                                renderThermalGraph(g_thermalMonitor);
                                ImGui::EndTabItem();
                            }
                            
                            ImGui::EndTabBar();
                        }
                        
                        ImGui::EndTabItem();
                    }
                    
                    // Memory & Processes Tab
                    if (ImGui::BeginTabItem("Memory & Processes")) {
                        g_activeMainTab = 2;
                        
                        // Only update when this tab is active for performance
                        renderMemoryProcessInterface(g_memoryProcessMonitor);
                        ImGui::EndTabItem();
                    }
                    
                    // Network Tab
                    if (ImGui::BeginTabItem("Network")) {
                        g_activeMainTab = 3;
                        
                        // Update network monitor
                        updateNetworkMonitor(g_networkMonitor);
                        
                        // Render network interface
                        renderNetworkInterface(g_networkMonitor);
                        ImGui::EndTabItem();
                    }
                    
                    // Enhanced Historical Data Tab
                    if (ImGui::BeginTabItem("Historical Data")) {
                        g_activeMainTab = 4;
                        
                        renderHistoricalGraphs(getHistoricalData());
                        ImGui::EndTabItem();
                    }
                    
                    // Enhanced Alerts Tab
                    if (ImGui::BeginTabItem("Alerts & Trends")) {
                        g_activeMainTab = 5;
                        
                        renderAdvancedSystemOverview(getHistoricalData(), g_cpuMonitor, 
                                                    g_memoryProcessMonitor, g_thermalMonitor);
                        renderAlertsInterface(getHistoricalData());
                        renderTrendAnalysisInterface(getHistoricalData());
                        ImGui::EndTabItem();
                    }
                    
                    // Enhanced Configuration & Export Tab
                    if (ImGui::BeginTabItem("Configuration")) {
                        g_activeMainTab = 6;
                        
                        renderConfigurationInterface(getHistoricalData().config);
                        renderExportInterface(getHistoricalData());
                        ImGui::EndTabItem();
                    }
                    
                    ImGui::EndTabBar();
                }
                
            }
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

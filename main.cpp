#include "header.h"
#include <SDL.h>
#include <algorithm>
#include <string>
#include <cctype>

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

// systemWindow, display information for the system monitorization
void systemWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    // System information
    ImGui::Text("Operating System: %s", getOsName());
    ImGui::Text("User: %s", getUsername().c_str());
    ImGui::Text("Hostname: %s", getHostname().c_str());
    
    TaskCounts tasks = getTaskCounts();
    ImGui::Text("Total Tasks: %d", tasks.total);
    ImGui::Text("Running: %d, Sleeping: %d, Stopped: %d, Zombie: %d", 
                tasks.running, tasks.sleeping, tasks.stopped, tasks.zombie);
    
    ImGui::Text("CPU: %s", CPUinfo().c_str());
    
    ImGui::Separator();
    
    // Tabbed section for CPU, Fan, Thermal
    if (ImGui::BeginTabBar("SystemTabs")) {
        
        // CPU Tab
        if (ImGui::BeginTabItem("CPU")) {
            static vector<float> cpuHistory;
            static bool animate = true;
            static float fps = 5.0f;  // Default to 5 FPS for reasonable update rate
            static float yScale = 100.0f;
            static Uint32 lastUpdateTime = 0;
            static float lastCpuUsage = 0.0f;

            Uint32 currentTime = SDL_GetTicks();
            float updateInterval = 1000.0f / fps;  // Convert FPS to milliseconds
            
            // Only update CPU usage at the specified FPS rate
            if (currentTime - lastUpdateTime >= updateInterval) {
                lastCpuUsage = calculateCPUUsage();
                lastUpdateTime = currentTime;
                
                if (animate) {
                    cpuHistory.push_back(lastCpuUsage);
                    if (cpuHistory.size() > 100) {
                        cpuHistory.erase(cpuHistory.begin());
                    }
                }
            }
            
            ImGui::Checkbox("Animate", &animate);
            ImGui::SliderFloat("FPS", &fps, 1.0f, 120.0f);
            ImGui::SliderFloat("Y Scale", &yScale, 50.0f, 200.0f);
            
            if (!cpuHistory.empty()) {
                ImGui::PlotLines("CPU Usage", cpuHistory.data(), cpuHistory.size(), 0, 
                               ("CPU: " + to_string((int)lastCpuUsage) + "%").c_str(), 0.0f, yScale, ImVec2(0, 80));
            }
            
            ImGui::EndTabItem();
        }
        
        // Fan Tab
        if (ImGui::BeginTabItem("Fan")) {
            static vector<float> fanHistory;
            static bool animate = true;
            static float fps = 5.0f;  // Default to 5 FPS for reasonable update rate
            static float yScale = 100.0f;
            static Uint32 lastUpdateTime = 0;
            static FanInfo lastFanInfo = {false, 0, 0};
            
            Uint32 currentTime = SDL_GetTicks();
            float updateInterval = 1000.0f / fps;  // Convert FPS to milliseconds
            
            // Only update fan info at the specified FPS rate
            if (currentTime - lastUpdateTime >= updateInterval) {
                lastFanInfo = getFanInfo();
                lastUpdateTime = currentTime;
                
                if (animate) {
                    fanHistory.push_back((float)lastFanInfo.speed);
                    if (fanHistory.size() > 100) {
                        fanHistory.erase(fanHistory.begin());
                    }
                }
            }
            
            ImGui::Text("Status: %s", lastFanInfo.enabled ? "Enabled" : "Disabled");
            ImGui::Text("Speed: %d RPM", lastFanInfo.speed);
            ImGui::Text("Level: %d", lastFanInfo.level);
            
            ImGui::Checkbox("Animate", &animate);
            ImGui::SliderFloat("FPS", &fps, 1.0f, 120.0f);
            ImGui::SliderFloat("Y Scale", &yScale, 50.0f, 200.0f);
            
            if (!fanHistory.empty()) {
                ImGui::PlotLines("Fan Speed", fanHistory.data(), fanHistory.size(), 0, 
                               ("Speed: " + to_string(lastFanInfo.speed) + " RPM").c_str(), 0.0f, yScale, ImVec2(0, 80));
            }
            
            ImGui::EndTabItem();
        }
        
        // Thermal Tab
        if (ImGui::BeginTabItem("Thermal")) {
            static vector<float> thermalHistory;
            static bool animate = true;
            static float fps = 5.0f;  // Default to 5 FPS for reasonable update rate
            static float yScale = 100.0f;
            static Uint32 lastUpdateTime = 0;
            static float lastTemp = 0.0f;
            
            Uint32 currentTime = SDL_GetTicks();
            float updateInterval = 1000.0f / fps;  // Convert FPS to milliseconds
            
            // Only update temperature at the specified FPS rate
            if (currentTime - lastUpdateTime >= updateInterval) {
                lastTemp = getThermalTemp();
                lastUpdateTime = currentTime;
                
                if (animate) {
                    thermalHistory.push_back(lastTemp);
                    if (thermalHistory.size() > 100) {
                        thermalHistory.erase(thermalHistory.begin());
                    }
                }
            }
            
            ImGui::Checkbox("Animate", &animate);
            ImGui::SliderFloat("FPS", &fps, 1.0f, 120.0f);
            ImGui::SliderFloat("Y Scale", &yScale, 50.0f, 200.0f);
            
            if (!thermalHistory.empty()) {
                ImGui::PlotLines("Temperature", thermalHistory.data(), thermalHistory.size(), 0, 
                               ("Temp: " + to_string((int)lastTemp) + "°C").c_str(), 0.0f, yScale, ImVec2(0, 80));
            }
            
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }

    ImGui::End();
}

// memoryProcessesWindow, display information for the memory and processes information
void memoryProcessesWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    // Memory information
    MemInfo ram = getMemInfo();
    MemInfo swap = getSwapInfo();
    DiskInfo disk = getDiskInfo();
    
    // RAM usage
    ImGui::Text("Physical Memory (RAM)");
    float ramUsage = (float)ram.used / ram.total;
    ImGui::ProgressBar(ramUsage, ImVec2(0.0f, 0.0f), (formatBytes(ram.used) + " / " + formatBytes(ram.total)).c_str());
    
    // SWAP usage
    ImGui::Text("Virtual Memory (SWAP)");
    float swapUsage = swap.total > 0 ? (float)swap.used / swap.total : 0.0f;
    ImGui::ProgressBar(swapUsage, ImVec2(0.0f, 0.0f), (formatBytes(swap.used) + " / " + formatBytes(swap.total)).c_str());
    
    // Disk usage
    ImGui::Text("Disk Usage");
    float diskUsage = (float)disk.used / disk.total;
    ImGui::ProgressBar(diskUsage, ImVec2(0.0f, 0.0f), (formatBytes(disk.used) + " / " + formatBytes(disk.total)).c_str());
    
    ImGui::Separator();
    
    // Process table
    static char filter[256] = "";
    static vector<int> selectedRows;
    
    ImGui::Text("Process Filter:");
    ImGui::InputText("##filter", filter, sizeof(filter));
    
    if (ImGui::BeginTabBar("ProcessTabs")) {
        if (ImGui::BeginTabItem("Processes")) {
            
            vector<Proc> processes = getProcesses();
            
            if (ImGui::BeginTable("ProcessTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
                ImGui::TableSetupColumn("PID");
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("State");
                ImGui::TableSetupColumn("CPU %");
                ImGui::TableSetupColumn("Memory %");
                ImGui::TableHeadersRow();
                
                string filterStr = string(filter);
                transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);
                
                for (size_t i = 0; i < processes.size(); i++) {
                    const Proc& proc = processes[i];
                    
                    // Apply filter
                    if (!filterStr.empty()) {
                        string procName = proc.name;
                        transform(procName.begin(), procName.end(), procName.begin(), ::tolower);
                        if (procName.find(filterStr) == string::npos) {
                            continue;
                        }
                    }
                    
                    ImGui::TableNextRow();
                    
                    // Selectable row
                    ImGui::TableSetColumnIndex(0);
                    bool isSelected = find(selectedRows.begin(), selectedRows.end(), i) != selectedRows.end();
                    if (ImGui::Selectable(("##row" + to_string(i)).c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                        if (ImGui::GetIO().KeyCtrl) {
                            if (isSelected) {
                                selectedRows.erase(remove(selectedRows.begin(), selectedRows.end(), i), selectedRows.end());
                            } else {
                                selectedRows.push_back(i);
                            }
                        } else {
                            selectedRows.clear();
                            selectedRows.push_back(i);
                        }
                    }
                    
                    ImGui::SameLine();
                    ImGui::Text("%d", proc.pid);
                    
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", proc.name.c_str());
                    
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%c", proc.state);
                    
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%.1f", proc.cpu_percent);
                    
                    ImGui::TableSetColumnIndex(4);
                    float memPercent = ram.total > 0 ? (float)(proc.rss * 4096) / ram.total * 100.0f : 0.0f;
                    ImGui::Text("%.1f", memPercent);
                }
                
                ImGui::EndTable();
            }
            
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

// network, display information network information
void networkWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    // Network interfaces and IP addresses
    Networks networks = getNetworks();
    NetStats stats = getNetStats();
    
    ImGui::Text("Network Interfaces:");
    for (const auto& ip4 : networks.ip4s) {
        ImGui::Text("%s: %s", ip4.name, ip4.addressBuffer);
    }
    
    ImGui::Separator();
    
    // Network statistics tables
    if (ImGui::BeginTabBar("NetworkTabs")) {
        
        // RX Table
        if (ImGui::BeginTabItem("RX (Receiver)")) {
            if (ImGui::BeginTable("RXTable", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Interface");
                ImGui::TableSetupColumn("Bytes");
                ImGui::TableSetupColumn("Packets");
                ImGui::TableSetupColumn("Errs");
                ImGui::TableSetupColumn("Drop");
                ImGui::TableSetupColumn("Fifo");
                ImGui::TableSetupColumn("Frame");
                ImGui::TableSetupColumn("Compressed");
                ImGui::TableHeadersRow();
                
                for (const auto& pair : stats.rx) {
                    const string& iface = pair.first;
                    const RX& rx = pair.second;
                    
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%s", iface.c_str());
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%lld", rx.bytes);
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%lld", rx.packets);
                    ImGui::TableSetColumnIndex(3); ImGui::Text("%lld", rx.errs);
                    ImGui::TableSetColumnIndex(4); ImGui::Text("%lld", rx.drop);
                    ImGui::TableSetColumnIndex(5); ImGui::Text("%lld", rx.fifo);
                    ImGui::TableSetColumnIndex(6); ImGui::Text("%lld", rx.frame);
                    ImGui::TableSetColumnIndex(7); ImGui::Text("%lld", rx.compressed);
                }
                
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        
        // TX Table
        if (ImGui::BeginTabItem("TX (Transmitter)")) {
            if (ImGui::BeginTable("TXTable", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Interface");
                ImGui::TableSetupColumn("Bytes");
                ImGui::TableSetupColumn("Packets");
                ImGui::TableSetupColumn("Errs");
                ImGui::TableSetupColumn("Drop");
                ImGui::TableSetupColumn("Fifo");
                ImGui::TableSetupColumn("Colls");
                ImGui::TableSetupColumn("Carrier");
                ImGui::TableHeadersRow();
                
                for (const auto& pair : stats.tx) {
                    const string& iface = pair.first;
                    const TX& tx = pair.second;
                    
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%s", iface.c_str());
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%lld", tx.bytes);
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%lld", tx.packets);
                    ImGui::TableSetColumnIndex(3); ImGui::Text("%lld", tx.errs);
                    ImGui::TableSetColumnIndex(4); ImGui::Text("%lld", tx.drop);
                    ImGui::TableSetColumnIndex(5); ImGui::Text("%lld", tx.fifo);
                    ImGui::TableSetColumnIndex(6); ImGui::Text("%lld", tx.colls);
                    ImGui::TableSetColumnIndex(7); ImGui::Text("%lld", tx.carrier);
                }
                
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::Separator();
    
    // Visual network usage
    if (ImGui::BeginTabBar("NetworkUsageTabs")) {
        
        // RX Usage
        if (ImGui::BeginTabItem("RX Usage")) {
            ImGui::Text("Network Receiver Usage");
            
            // Find max bytes for scaling
            long long maxBytes = 1024 * 1024; // Minimum 1MB for scaling
            for (const auto& pair : stats.rx) {
                if (pair.second.bytes > maxBytes) {
                    maxBytes = pair.second.bytes;
                }
            }
            
            for (const auto& pair : stats.rx) {
                const string& iface = pair.first;
                const RX& rx = pair.second;
                
                float usage = maxBytes > 0 ? (float)rx.bytes / maxBytes : 0.0f;
                
                ImGui::Text("%s:", iface.c_str());
                ImGui::ProgressBar(usage, ImVec2(0.0f, 0.0f), formatBytes(rx.bytes).c_str());
            }
            
            ImGui::EndTabItem();
        }
        
        // TX Usage
        if (ImGui::BeginTabItem("TX Usage")) {
            ImGui::Text("Network Transmitter Usage");
            
            // Find max bytes for scaling
            long long maxBytes = 1024 * 1024; // Minimum 1MB for scaling
            for (const auto& pair : stats.tx) {
                if (pair.second.bytes > maxBytes) {
                    maxBytes = pair.second.bytes;
                }
            }
            
            for (const auto& pair : stats.tx) {
                const string& iface = pair.first;
                const TX& tx = pair.second;
                
                float usage = maxBytes > 0 ? (float)tx.bytes / maxBytes : 0.0f;
                
                ImGui::Text("%s:", iface.c_str());
                ImGui::ProgressBar(usage, ImVec2(0.0f, 0.0f), formatBytes(tx.bytes).c_str());
            }
            
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }

    ImGui::End();
}

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
    SDL_Window *window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
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

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

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
            ImVec2 mainDisplay = io.DisplaySize;
            memoryProcessesWindow("== Memory and Processes ==",
                                  ImVec2((mainDisplay.x / 2) - 20, (mainDisplay.y / 2) + 30),
                                  ImVec2((mainDisplay.x / 2) + 10, 10));
            // --------------------------------------
            systemWindow("== System ==",
                         ImVec2((mainDisplay.x / 2) - 10, (mainDisplay.y / 2) + 30),
                         ImVec2(10, 10));
            // --------------------------------------
            networkWindow("== Network ==",
                          ImVec2(mainDisplay.x - 20, (mainDisplay.y / 2) - 60),
                          ImVec2(10, (mainDisplay.y / 2) + 50));
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

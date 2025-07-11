#include "header.h"
#include <SDL2/SDL.h>

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

    // System Info Section
    ImGui::Text("System Information");
    ImGui::Separator();
    
    // Basic system info
    ImGui::Text("OS: %s", getOsName());
    ImGui::Text("User: %s", getCurrentUser().c_str());
    ImGui::Text("Hostname: %s", getHostname().c_str());
    ImGui::Text("Total Processes: %d", getTotalProcesses());
    ImGui::Text("CPU: %s", CPUinfo().c_str());
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Tabbed sections
    if (ImGui::BeginTabBar("SystemTabs"))
    {
        // CPU Tab
        if (ImGui::BeginTabItem("CPU"))
        {
            static bool pauseCPU = false;
            static float cpuGraphFPS = 60.0f;
            static float cpuYScale = 1.0f;
            static vector<float> cpuHistory;
            static CPUStats previousCPU = {0};
            
            // Controls
            ImGui::Checkbox("Pause CPU Graph", &pauseCPU);
            ImGui::SameLine();
            ImGui::SliderFloat("FPS", &cpuGraphFPS, 1.0f, 120.0f, "%.1f");
            ImGui::SliderFloat("Y-Scale", &cpuYScale, 0.1f, 2.0f, "%.1f");
            
            // Read current CPU stats
            CPUStats currentCPU = readCPUStats();
            float cpuPercent = calculateCPUPercent(currentCPU, previousCPU);
            previousCPU = currentCPU;
            
            // Update history if not paused
            if (!pauseCPU)
            {
                cpuHistory.push_back(cpuPercent);
                if (cpuHistory.size() > 100)
                    cpuHistory.erase(cpuHistory.begin());
            }
            
            // Display current CPU usage
            ImGui::Text("CPU Usage: %.1f%%", cpuPercent);
            
            // CPU Graph
            if (!cpuHistory.empty())
            {
                ImGui::PlotLines("CPU Usage", cpuHistory.data(), cpuHistory.size(), 0, 
                               ("CPU: " + to_string((int)cpuPercent) + "%").c_str(), 
                               0.0f, 100.0f * cpuYScale, ImVec2(0, 80));
            }
            
            ImGui::EndTabItem();
        }
        
        // Fan Tab
        if (ImGui::BeginTabItem("Fan"))
        {
            static bool pauseFan = false;
            static float fanGraphFPS = 60.0f;
            static float fanYScale = 1.0f;
            static vector<float> fanHistory;
            
            // Controls
            ImGui::Checkbox("Pause Fan Graph", &pauseFan);
            ImGui::SameLine();
            ImGui::SliderFloat("FPS##Fan", &fanGraphFPS, 1.0f, 120.0f, "%.1f");
            ImGui::SliderFloat("Y-Scale##Fan", &fanYScale, 0.1f, 2.0f, "%.1f");
            
            // Simulated fan speed (since fan info is hardware-specific)
            static float fanSpeed = 1200.0f + (rand() % 400);
            
            if (!pauseFan)
            {
                fanSpeed = 1200.0f + (rand() % 400);
                fanHistory.push_back(fanSpeed);
                if (fanHistory.size() > 100)
                    fanHistory.erase(fanHistory.begin());
            }
            
            ImGui::Text("Fan Speed: %.0f RPM", fanSpeed);
            
            // Fan Graph
            if (!fanHistory.empty())
            {
                ImGui::PlotLines("Fan Speed", fanHistory.data(), fanHistory.size(), 0,
                               ("Fan: " + to_string((int)fanSpeed) + " RPM").c_str(),
                               800.0f, 2000.0f * fanYScale, ImVec2(0, 80));
            }
            
            ImGui::EndTabItem();
        }
        
        // Thermal Tab
        if (ImGui::BeginTabItem("Thermal"))
        {
            static bool pauseThermal = false;
            static float thermalGraphFPS = 60.0f;
            static float thermalYScale = 1.0f;
            static vector<float> thermalHistory;
            
            // Controls
            ImGui::Checkbox("Pause Thermal Graph", &pauseThermal);
            ImGui::SameLine();
            ImGui::SliderFloat("FPS##Thermal", &thermalGraphFPS, 1.0f, 120.0f, "%.1f");
            ImGui::SliderFloat("Y-Scale##Thermal", &thermalYScale, 0.1f, 2.0f, "%.1f");
            
            // Read thermal temperature
            float temp = readThermalTemp();
            
            if (!pauseThermal)
            {
                thermalHistory.push_back(temp);
                if (thermalHistory.size() > 100)
                    thermalHistory.erase(thermalHistory.begin());
            }
            
            ImGui::Text("Temperature: %.1f°C", temp);
            
            // Thermal Graph
            if (!thermalHistory.empty())
            {
                ImGui::PlotLines("Temperature", thermalHistory.data(), thermalHistory.size(), 0,
                               ("Temp: " + to_string((int)temp) + "°C").c_str(),
                               0.0f, 100.0f * thermalYScale, ImVec2(0, 80));
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

    // Memory Information
    MemoryInfo memInfo = readMemoryInfo();
    
    ImGui::Text("Memory Information");
    ImGui::Separator();
    
    // RAM Section
    ImGui::Text("RAM Usage");
    ImGui::Text("Total: %s", formatBytes(memInfo.total).c_str());
    ImGui::Text("Used: %s (%.1f%%)", formatBytes(memInfo.used).c_str(), memInfo.memUsedPercent);
    ImGui::Text("Available: %s", formatBytes(memInfo.available).c_str());
    ImGui::Text("Cached: %s", formatBytes(memInfo.cached).c_str());
    ImGui::Text("Buffers: %s", formatBytes(memInfo.buffers).c_str());
    
    // RAM Progress Bar
    ImGui::ProgressBar(memInfo.memUsedPercent / 100.0f, ImVec2(0.0f, 0.0f), 
                      ("RAM: " + to_string((int)memInfo.memUsedPercent) + "%").c_str());
    
    ImGui::Spacing();
    
    // SWAP Section
    ImGui::Text("SWAP Usage");
    ImGui::Text("Total: %s", formatBytes(memInfo.swapTotal).c_str());
    ImGui::Text("Used: %s (%.1f%%)", formatBytes(memInfo.swapUsed).c_str(), memInfo.swapUsedPercent);
    ImGui::Text("Free: %s", formatBytes(memInfo.swapFree).c_str());
    
    // SWAP Progress Bar
    ImGui::ProgressBar(memInfo.swapUsedPercent / 100.0f, ImVec2(0.0f, 0.0f),
                      ("SWAP: " + to_string((int)memInfo.swapUsedPercent) + "%").c_str());
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Disk Information
    ImGui::Text("Disk Usage");
    vector<DiskInfo> disks = readDiskInfo();
    
    for (const auto& disk : disks)
    {
        ImGui::Text("%s (%s)", disk.filesystem.c_str(), disk.mountpoint.c_str());
        ImGui::Text("Total: %s, Used: %s, Free: %s", 
                   formatBytes(disk.total).c_str(),
                   formatBytes(disk.used).c_str(),
                   formatBytes(disk.free).c_str());
        ImGui::ProgressBar(disk.usedPercent / 100.0f, ImVec2(0.0f, 0.0f),
                          ("Disk: " + to_string((int)disk.usedPercent) + "%").c_str());
        ImGui::Spacing();
    }
    
    ImGui::Separator();
    
    // Process Table
    ImGui::Text("Process Table");
    static char processFilter[256] = "";
    ImGui::InputText("Filter processes", processFilter, sizeof(processFilter));
    
    static vector<Proc> processes;
    static float lastUpdate = 0.0f;
    float currentTime = ImGui::GetTime();
    
    // Update process list every 2 seconds
    if (currentTime - lastUpdate > 2.0f)
    {
        processes = readProcessList();
        lastUpdate = currentTime;
    }
    
    // Process table headers
    if (ImGui::BeginTable("ProcessTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
    {
        ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Virtual Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("RSS", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("CPU Time", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableHeadersRow();
        
        string filterStr = string(processFilter);
        transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);
        
        for (const auto& proc : processes)
        {
            // Filter processes
            if (!filterStr.empty())
            {
                string procName = proc.name;
                transform(procName.begin(), procName.end(), procName.begin(), ::tolower);
                if (procName.find(filterStr) == string::npos)
                    continue;
            }
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%d", proc.pid);
            
            ImGui::TableNextColumn();
            ImGui::Text("%s", proc.name.c_str());
            
            ImGui::TableNextColumn();
            ImGui::Text("%c", proc.state);
            
            ImGui::TableNextColumn();
            ImGui::Text("%s", formatBytes(proc.vsize).c_str());
            
            ImGui::TableNextColumn();
            ImGui::Text("%s", formatBytes(proc.rss * 4096).c_str()); // RSS is in pages
            
            ImGui::TableNextColumn();
            ImGui::Text("%lld", proc.utime + proc.stime);
        }
        
        ImGui::EndTable();
    }

    ImGui::End();
}

// network, display information network information
void networkWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    ImGui::Text("Network Information");
    ImGui::Separator();
    
    // IPv4 Interfaces
    ImGui::Text("IPv4 Interfaces");
    Networks networks = getNetworkInterfaces();
    
    if (ImGui::BeginTable("IPv4Table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Interface", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("IPv4 Address", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableHeadersRow();
        
        for (const auto& ip4 : networks.ip4s)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", ip4.name);
            
            ImGui::TableNextColumn();
            ImGui::Text("%s", ip4.addressBuffer);
        }
        
        ImGui::EndTable();
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Network Statistics
    ImGui::Text("Network Statistics");
    static vector<NetworkStats> networkStats;
    static float lastNetworkUpdate = 0.0f;
    float currentTime = ImGui::GetTime();
    
    // Update network stats every 2 seconds
    if (currentTime - lastNetworkUpdate > 2.0f)
    {
        networkStats = readNetworkStats();
        lastNetworkUpdate = currentTime;
    }
    
    // Interface selector
    static size_t selectedInterface = 0;
    vector<string> interfaceNames = getNetworkInterfaceList();
    
    if (!interfaceNames.empty())
    {
        ImGui::Text("Select Interface:");
        ImGui::SameLine();
        if (ImGui::BeginCombo("##interface", interfaceNames[selectedInterface].c_str()))
        {
            for (size_t i = 0; i < interfaceNames.size(); i++)
            {
                bool isSelected = (selectedInterface == i);
                if (ImGui::Selectable(interfaceNames[i].c_str(), isSelected))
                    selectedInterface = i;
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        
        ImGui::Spacing();
        
        // RX Statistics Table
        ImGui::Text("RX (Receive) Statistics for %s", interfaceNames[selectedInterface].c_str());
        RX rxStats = getRXStats(interfaceNames[selectedInterface]);
        
        if (ImGui::BeginTable("RXTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Metric", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 200.0f);
            ImGui::TableHeadersRow();
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Bytes");
            ImGui::TableNextColumn();
            ImGui::Text("%s", formatNetworkBytes(rxStats.bytes).c_str());
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Packets");
            ImGui::TableNextColumn();
            ImGui::Text("%d", rxStats.packets);
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Errors");
            ImGui::TableNextColumn();
            ImGui::Text("%d", rxStats.errs);
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Dropped");
            ImGui::TableNextColumn();
            ImGui::Text("%d", rxStats.drop);
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("FIFO");
            ImGui::TableNextColumn();
            ImGui::Text("%d", rxStats.fifo);
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Compressed");
            ImGui::TableNextColumn();
            ImGui::Text("%d", rxStats.compressed);
            
            ImGui::EndTable();
        }
        
        ImGui::Spacing();
        
        // TX Statistics Table
        ImGui::Text("TX (Transmit) Statistics for %s", interfaceNames[selectedInterface].c_str());
        TX txStats = getTXStats(interfaceNames[selectedInterface]);
        
        if (ImGui::BeginTable("TXTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Metric", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 200.0f);
            ImGui::TableHeadersRow();
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Bytes");
            ImGui::TableNextColumn();
            ImGui::Text("%s", formatNetworkBytes(txStats.bytes).c_str());
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Packets");
            ImGui::TableNextColumn();
            ImGui::Text("%d", txStats.packets);
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Errors");
            ImGui::TableNextColumn();
            ImGui::Text("%d", txStats.errs);
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Dropped");
            ImGui::TableNextColumn();
            ImGui::Text("%d", txStats.drop);
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("FIFO");
            ImGui::TableNextColumn();
            ImGui::Text("%d", txStats.fifo);
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Frame");
            ImGui::TableNextColumn();
            ImGui::Text("%d", txStats.frame);
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Compressed");
            ImGui::TableNextColumn();
            ImGui::Text("%d", txStats.compressed);
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Multicast");
            ImGui::TableNextColumn();
            ImGui::Text("%d", txStats.multicast);
            
            ImGui::EndTable();
        }
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

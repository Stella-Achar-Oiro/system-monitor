# Desktop System Monitor

A comprehensive real-time system monitoring application built with C++ and Dear ImGui, designed to display detailed system information including CPU, memory, network, and process statistics.

## Features

### System Information
- **Operating System**: Displays current OS name
- **User Information**: Shows currently logged-in user
- **Hostname**: System hostname display
- **Process Statistics**: Real-time process counts by state (running, sleeping, zombie, etc.)
- **CPU Information**: Detailed CPU model and specifications

### System Monitor (Tabbed Interface)
- **CPU Tab**: Real-time CPU usage with interactive performance graph
- **Fan Tab**: Hardware fan monitoring with speed and level indicators
- **Thermal Tab**: Temperature monitoring with thermal sensor data
- **Interactive Controls**: All graphs feature pause/resume, FPS adjustment (1-120 FPS), and Y-scale controls (50-200 range)

### Memory & Process Monitor
- **RAM Usage**: Visual progress bar with detailed memory statistics
- **SWAP Usage**: Virtual memory monitoring with usage percentages
- **Disk Usage**: Filesystem space monitoring for mounted drives
- **Process Table**: Comprehensive process list with:
  - PID, Name, State
  - CPU Usage Percentage
  - Memory Usage Percentage
  - **Process filtering** (search by name)
  - **Basic selection** support

### Network Monitor
- **IPv4 Interfaces**: Display of all network interfaces with IP addresses
- **RX Statistics**: Receive statistics including bytes, packets, errors, drops, etc.
- **TX Statistics**: Transmit statistics with collision and carrier error tracking
- **Visual Usage Display**: Progress bars showing network usage with dynamic scaling
- **Byte Formatting**: Automatic byte conversion (B/K/M/G) with standard 1024-based scaling

## Technical Implementation

### Architecture
- **Language**: C++ with modern STL features
- **GUI Framework**: Dear ImGui with OpenGL3 backend
- **System Integration**: Direct `/proc` filesystem access for real-time data
- **Cross-platform**: Linux-focused with potential for cross-platform expansion

### Data Sources
- `/proc/stat` - CPU statistics
- `/proc/meminfo` - Memory information
- `/proc/net/dev` - Network interface statistics
- `/proc/[pid]/stat` - Process information
- `/sys/class/thermal/thermal_zone0/temp` - Temperature sensors
- `/sys/class/hwmon/hwmon*/fan1_input` - Fan speed monitoring

### Key Components
- **system.cpp**: System information and hardware monitoring
- **mem.cpp**: Memory management and process tracking
- **network.cpp**: Network interface monitoring and statistics
- **main.cpp**: ImGui interface and application loop

## Building and Installation

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt install libsdl2-dev build-essential

# The project includes all ImGui dependencies
```

### Build Process
```bash
# Clone or extract the project
cd system-monitor

# Build the application
make

# Run the system monitor
./monitor
```

### Build Configuration
The project uses a cross-platform Makefile with support for:
- **Linux**: OpenGL + SDL2
- **macOS**: OpenGL + SDL2 with framework support
- **Windows**: MinGW with static linking

## Usage

### Running the Application
```bash
./monitor
```

### Interface Navigation
1. **System Window**: Basic system information and tabbed monitoring
2. **Memory & Processes Window**: Memory usage and process management
3. **Network Window**: Network interface monitoring and statistics

### Interactive Features
- **Graph Controls**: Use sliders to adjust FPS (1-120) and Y-scale (50-200)
- **Pause/Resume**: Stop graph animations for detailed analysis
- **Process Filtering**: Type in the filter box to search processes by name
- **Process Selection**: Basic row selection with Ctrl+click support

## Performance Monitoring

### Real-time Updates
- **System Stats**: Updated every frame
- **Process Table**: Updated on window refresh
- **Network Stats**: Updated on window refresh
- **Graphs**: 5 FPS default (adjustable 1-120 FPS)

### Resource Usage
- **CPU**: Minimal impact, optimized /proc reading
- **Memory**: Low footprint with efficient data structures
- **Network**: No additional network traffic (local monitoring only)

## Troubleshooting

### Common Issues
1. **Fan monitoring unavailable**: Normal on systems without accessible hwmon
2. **Temperature reading fails**: Check thermal sensor availability
3. **Network interfaces missing**: Verify /proc/net/dev accessibility
4. **Build errors**: Ensure SDL2 development packages are installed

### Debug Mode
```bash
# Build with debug symbols
make clean && make
```

### System Requirements
- **OS**: Linux (tested on Ubuntu/Debian)
- **Memory**: 50MB RAM minimum
- **Dependencies**: SDL2, OpenGL
- **Privileges**: No root access required

## License and Credits

This project demonstrates advanced C++ programming with system-level monitoring capabilities. It showcases:
- Modern C++ practices and STL usage
- Real-time GUI development with Dear ImGui
- Linux system programming with /proc filesystem
- Performance optimization and memory management
- Cross-platform build system design

## Development Notes

### Code Organization
- **Modular Design**: Separate modules for different monitoring aspects
- **Error Handling**: Graceful degradation when hardware features unavailable
- **Data Validation**: Input sanitization and bounds checking
- **Performance**: Optimized data structures and update cycles

### Future Enhancements
- GPU monitoring support
- Network bandwidth graphing
- Process control features
- Configuration file support
- Plugin architecture for custom monitors

---

**Note**: This system monitor provides comprehensive real-time monitoring without requiring elevated privileges, making it safe for regular user operation while providing detailed system insights.
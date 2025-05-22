# Professional WiFi Scanner

A sophisticated Wi-Fi network scanner built entirely in C using the Windows API, featuring a modern GUI with real-time updates, signal strength visualization, and comprehensive network analysis.

## Features

### 🎯 **Core Functionality**
- **Real-time Wi-Fi scanning** - Automatic updates every 3 seconds
- **Comprehensive network detection** - Discovers all available networks including hidden ones
- **Signal strength analysis** - Visual bar graphs and detailed dBm readings
- **Security protocol identification** - Detects WEP, WPA, WPA2, WPA3, Enhanced Open (OWE)
- **Network connection status** - Shows currently connected network
- **Channel and frequency information** - Detailed technical specifications

### 🎨 **Professional GUI**
- **Modern dark theme** - Easy on the eyes with professional appearance
- **Tree view network listing** - Hierarchical display with expandable details
- **Real-time signal graphs** - Visual representation of network strength
- **Custom-drawn components** - Gradient buttons, rounded corners, smooth animations
- **Responsive layout** - Automatically adjusts to window resizing
- **Status bar updates** - Real-time scanning progress and statistics

### 💾 **Data Management**
- **CSV export functionality** - Save scan results with detailed network information
- **Comprehensive data capture** - SSID, BSSID, signal strength, security, channel, frequency
- **Duplicate filtering** - Intelligent removal of redundant network entries
- **Sorted results** - Networks ordered by signal strength for easy analysis

### ⚡ **Performance & Reliability**
- **Memory leak prevention** - Careful resource management and cleanup
- **Efficient scanning** - Optimized Windows WLAN API usage
- **Error handling** - Robust error detection and user feedback
- **Thread-safe operations** - Prevents UI freezing during scans

## Technical Details

### Architecture
The application is built using a modular architecture with three main components:

1. **main.c** - Main application logic, GUI management, and event handling
2. **wifi_scanner.c** - Wi-Fi scanning engine using Windows WLAN API
3. **gui_components.c** - Custom GUI components and modern styling

### Dependencies
- **Windows 10/11** - Required for WLAN API support
- **Visual Studio 2022** - For building the project
- **Windows SDK** - For Win32 API access

### API Usage
- **Windows WLAN API** - For Wi-Fi network discovery and management
- **Win32 GDI** - For custom drawing and graphics
- **Common Controls** - For tree view and status bar components

## Building the Project

### Prerequisites
1. **Visual Studio 2022** with C++ development tools
2. **Windows 10 SDK** (10.0 or later)
3. **Git** (for cloning the repository)

### Build Steps
1. Open `WifiScanner.sln` in Visual Studio 2022
2. Select your target configuration (Debug/Release)
3. Choose your platform (x86/x64)
4. Build the solution (Ctrl+Shift+B)

### Build Configurations
- **Debug (x86/x64)** - For development and debugging
- **Release (x86/x64)** - Optimized for production use

## Usage Instructions

### Running the Application
1. Launch `WifiScanner.exe`
2. The application will automatically start scanning for networks
3. Networks will appear in the tree view sorted by signal strength
4. The signal graph updates in real-time showing the strongest networks

### Interface Elements
- **Tree View (Left)** - Expandable list of detected networks with details
- **Signal Graph (Right)** - Visual representation of network signal strengths
- **Refresh Button** - Manual network scan trigger
- **Export Button** - Save results to CSV file
- **Status Bar (Bottom)** - Scan progress and network count

### Network Information
Each detected network displays:
- **SSID** - Network name (or "Hidden" for hidden networks)
- **Signal Strength** - dBm reading with quality assessment
- **BSSID** - MAC address of the access point
- **Security** - Encryption protocol (Open, WEP, WPA2, WPA3, etc.)
- **Channel** - Wi-Fi channel and frequency
- **Connection Status** - Shows if currently connected

### Export Features
- Click "Export Results" to save scan data
- Choose file location and name
- Data saved in CSV format for analysis in Excel or other tools
- Includes all detected network information with timestamps

## System Requirements

### Minimum Requirements
- **OS**: Windows 10 (1903 or later)
- **RAM**: 256 MB
- **Storage**: 50 MB free space
- **Wi-Fi**: Built-in or USB Wi-Fi adapter

### Recommended Requirements
- **OS**: Windows 11
- **RAM**: 512 MB
- **Storage**: 100 MB free space
- **Display**: 1920x1080 or higher resolution

## Security & Privacy

### Data Handling
- **No data transmission** - All scanning is performed locally
- **No network connection** - Application only reads Wi-Fi information
- **No password storage** - Security protocols detected but not passwords
- **Local file access only** - Export files saved to user-selected locations

### Permissions Required
- **Wi-Fi access** - Required for network scanning
- **File system access** - Only for export functionality
- **No administrative rights** - Runs with standard user permissions

## Troubleshooting

### Common Issues
1. **No networks detected**
   - Ensure Wi-Fi adapter is enabled
   - Check Windows Wi-Fi service is running
   - Verify adapter drivers are installed

2. **Application won't start**
   - Install Visual C++ Redistributable
   - Check Windows version compatibility
   - Run as administrator if needed

3. **Export fails**
   - Check file permissions in target directory
   - Ensure sufficient disk space
   - Verify antivirus is not blocking file creation

### Performance Tips
- **Close other Wi-Fi tools** - Prevents scanning conflicts
- **Update Wi-Fi drivers** - Ensures best compatibility
- **Regular Windows updates** - Keeps WLAN API current

## Development Notes

### Code Structure
The codebase follows professional C development practices:
- **Modular design** - Separated concerns across three files
- **Memory management** - Proper allocation and cleanup
- **Error handling** - Comprehensive error checking
- **Code documentation** - Clear function and variable naming

### Customization
The application can be easily customized:
- **Scan intervals** - Modify timer values in main.c
- **GUI colors** - Update color constants in gui_components.c
- **Window sizing** - Adjust dimensions in main.c
- **Export format** - Modify CSV output in main.c

## License

This project is created for educational and professional demonstration purposes. 
The code demonstrates advanced C programming techniques and Windows API usage.

## Author

Created as a professional demonstration of C programming capabilities with:
- Advanced Win32 API integration
- Modern GUI design principles
- Real-time data processing
- Professional software architecture

---

*This WiFi Scanner represents professional-grade C development with modern UI design and comprehensive network analysis capabilities.* 
/*
 * keypad-monitor - Monitor MX Keypad button events
 * 
 * Usage:
 *   keypad-monitor [OPTIONS]
 * 
 * Options:
 *   --json               Output events in JSON format (one per line)
 *   --grid-only          Only output grid button events (0-8)
 *   --nav-only           Only output navigation button events (P1/P2)
 *   --device PATH        Use specific device path
 *   --help               Show this help message
 */

#include <logilinux/logilinux.h>
#include <logilinux/device.h>
#include <logilinux/events.h>
#include <iostream>
#include <iomanip>
#include <atomic>
#include <csignal>
#include <string>
#include <thread>
#include <chrono>

std::atomic<bool> running(true);

void signalHandler(int) {
    running = false;
}

struct Options {
    bool json = false;
    bool gridOnly = false;
    bool navOnly = false;
    std::string devicePath;
};

void printHelp(const char* progName) {
    std::cout << "Usage: " << progName << " [OPTIONS]\n\n"
              << "Monitor button events from Logitech MX Keypad.\n\n"
              << "Options:\n"
              << "  --json               Output events in JSON format (one per line)\n"
              << "  --grid-only          Only output grid button events (0-8)\n"
              << "  --nav-only           Only output navigation button events (P1/P2)\n"
              << "  --device PATH        Use specific device path\n"
              << "  --help               Show this help message\n\n"
              << "Button Layout:\n"
              << "  Grid buttons:        GRID_0 through GRID_8 (3x3 grid, codes 0-8)\n"
              << "  Navigation buttons:  P1_LEFT (0xa1), P2_RIGHT (0xa2)\n\n"
              << "Output Format (JSON):\n"
              << "  {\"type\":\"button\",\"action\":\"press\",\"button\":\"GRID_0\",\"code\":0,\"timestamp\":1234567}\n"
              << "  {\"type\":\"button\",\"action\":\"release\",\"button\":\"P1_LEFT\",\"code\":161,\"timestamp\":1234567}\n\n"
              << "Examples:\n"
              << "  " << progName << "                     # Monitor all button events\n"
              << "  " << progName << " --json              # JSON output for scripting\n"
              << "  " << progName << " --grid-only         # Only grid buttons\n"
              << "  " << progName << " --nav-only          # Only P1/P2 buttons\n"
              << "  " << progName << " --json | jq .       # Pretty JSON with jq\n\n"
              << "Pipe to other commands:\n"
              << "  " << progName << " --json --grid-only | while read event; do\n"
              << "    button=$(echo $event | jq -r .button)\n"
              << "    # Process button press...\n"
              << "  done\n";
}

bool isNavigationButton(uint32_t code) {
    return code == 0xa1 || code == 0xa2;
}

void handleEvent(LogiLinux::EventPtr event, const Options& opts) {
    if (auto button = std::dynamic_pointer_cast<LogiLinux::ButtonEvent>(event)) {
        bool isNav = isNavigationButton(button->button_code);
        
        // Filter based on options
        if (opts.gridOnly && isNav) return;
        if (opts.navOnly && !isNav) return;
        
        auto keypadButton = LogiLinux::getMXKeypadButton(button->button_code);
        const char* buttonName = LogiLinux::getMXKeypadButtonName(keypadButton);
        
        if (opts.json) {
            std::cout << "{\"type\":\"button\""
                     << ",\"action\":\"" << (button->pressed ? "press" : "release") << "\""
                     << ",\"button\":\"" << buttonName << "\""
                     << ",\"code\":" << button->button_code
                     << ",\"timestamp\":" << button->timestamp
                     << "}" << std::endl;
        } else {
            std::cout << "[BUTTON] " << (button->pressed ? "PRESS  " : "RELEASE")
                     << " | " << std::setw(12) << buttonName
                     << " | Code: 0x" << std::hex << std::setw(2) << std::setfill('0') 
                     << button->button_code << std::dec << std::setfill(' ')
                     << " | Timestamp: " << button->timestamp << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    Options opts;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printHelp(argv[0]);
            return 0;
        } else if (arg == "--json") {
            opts.json = true;
        } else if (arg == "--grid-only") {
            opts.gridOnly = true;
        } else if (arg == "--nav-only") {
            opts.navOnly = true;
        } else if (arg == "--device") {
            if (i + 1 < argc) {
                opts.devicePath = argv[++i];
            } else {
                std::cerr << "Error: --device requires an argument" << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            std::cerr << "Use --help for usage information." << std::endl;
            return 1;
        }
    }
    
    // Check for conflicting options
    if (opts.gridOnly && opts.navOnly) {
        std::cerr << "Error: Cannot use --grid-only and --nav-only together" << std::endl;
        return 1;
    }
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    LogiLinux::Library lib;
    LogiLinux::DevicePtr keypad;
    
    // Find device
    if (!opts.devicePath.empty()) {
        auto devices = lib.discoverDevices();
        for (const auto& dev : devices) {
            if (dev->getType() == LogiLinux::DeviceType::MX_KEYPAD &&
                dev->getInfo().device_path == opts.devicePath) {
                keypad = dev;
                break;
            }
        }
        
        if (!keypad) {
            std::cerr << "Error: No MX Keypad found at " << opts.devicePath << std::endl;
            return 1;
        }
    } else {
        keypad = lib.findDevice(LogiLinux::DeviceType::MX_KEYPAD);
        
        if (!keypad) {
            std::cerr << "Error: No MX Keypad found" << std::endl;
            std::cerr << "Make sure device is connected and you have permissions." << std::endl;
            return 1;
        }
    }
    
    // Set up event callback
    keypad->setEventCallback([&opts](LogiLinux::EventPtr event) {
        handleEvent(event, opts);
    });
    
    // Start monitoring
    keypad->startMonitoring();
    
    if (!keypad->isMonitoring()) {
        std::cerr << "Error: Failed to start monitoring" << std::endl;
        std::cerr << "Try running with sudo if you get permission errors." << std::endl;
        return 1;
    }
    
    // Wait for events
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Cleanup
    keypad->stopMonitoring();
    
    return 0;
}

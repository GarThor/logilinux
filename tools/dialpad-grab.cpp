/*
 * dialpad-grab - Grab or release MX Dialpad device exclusively
 * 
 * Usage:
 *   dialpad-grab [OPTIONS] <grab|release>
 * 
 * Options:
 *   --device PATH        Use specific device path
 *   --help               Show this help message
 */

#include <logilinux/logilinux.h>
#include <logilinux/device.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

void printHelp(const char* progName) {
    std::cout << "Usage: " << progName << " [OPTIONS] <grab|release>\n\n"
              << "Grab or release MX Dialpad device exclusively.\n"
              << "When grabbed, the device's default behavior is disabled.\n\n"
              << "Options:\n"
              << "  --device PATH        Use specific device path (e.g., /dev/input/event5)\n"
              << "  --help               Show this help message\n\n"
              << "Arguments:\n"
              << "  grab                 Grab device exclusively\n"
              << "  release              Release exclusive grab\n\n"
              << "Examples:\n"
              << "  " << progName << " grab           # Disable default dialpad behavior\n"
              << "  " << progName << " release        # Re-enable default behavior\n\n"
              << "Note: Requires appropriate permissions (sudo or input group membership)\n";
}

int main(int argc, char* argv[]) {
    std::string devicePath;
    std::string action;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printHelp(argv[0]);
            return 0;
        } else if (arg == "--device") {
            if (i + 1 < argc) {
                devicePath = argv[++i];
            } else {
                std::cerr << "Error: --device requires an argument" << std::endl;
                return 1;
            }
        } else if (arg == "grab" || arg == "release") {
            if (!action.empty()) {
                std::cerr << "Error: Multiple actions specified" << std::endl;
                return 1;
            }
            action = arg;
        } else {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            std::cerr << "Use --help for usage information." << std::endl;
            return 1;
        }
    }
    
    // Validate action
    if (action.empty()) {
        std::cerr << "Error: No action specified (grab or release)" << std::endl;
        std::cerr << "Use --help for usage information." << std::endl;
        return 1;
    }
    
    bool shouldGrab = (action == "grab");
    
    // Find device
    LogiLinux::Library lib;
    LogiLinux::DevicePtr dialpad;
    
    if (!devicePath.empty()) {
        auto devices = lib.discoverDevices();
        for (const auto& dev : devices) {
            if (dev->getType() == LogiLinux::DeviceType::DIALPAD &&
                dev->getInfo().device_path == devicePath) {
                dialpad = dev;
                break;
            }
        }
        
        if (!dialpad) {
            std::cerr << "Error: No dialpad found at " << devicePath << std::endl;
            return 1;
        }
    } else {
        dialpad = lib.findDevice(LogiLinux::DeviceType::DIALPAD);
        
        if (!dialpad) {
            std::cerr << "Error: No MX Dialpad found" << std::endl;
            std::cerr << "Make sure device is connected and you have permissions." << std::endl;
            return 1;
        }
    }
    
    // Start monitoring first (device must be opened)
    dialpad->startMonitoring();
    
    if (!dialpad->isMonitoring()) {
        std::cerr << "Error: Failed to access device" << std::endl;
        std::cerr << "Try running with sudo if you get permission errors." << std::endl;
        return 1;
    }
    
    // Perform grab/release
    if (!dialpad->grabExclusive(shouldGrab)) {
        std::cerr << "Error: Failed to " << action << " device" << std::endl;
        std::cerr << "Try running with sudo for device control." << std::endl;
        dialpad->stopMonitoring();
        return 1;
    }
    
    std::cout << "Successfully " << (shouldGrab ? "grabbed" : "released") 
              << " device: " << dialpad->getInfo().device_path << std::endl;
    
    // Keep monitoring active if grabbed, otherwise stop
    if (shouldGrab) {
        std::cout << "Device is now grabbed exclusively. Default behavior disabled." << std::endl;
        std::cout << "Press Ctrl+C to release and exit." << std::endl;
        
        // Wait indefinitely
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } else {
        dialpad->stopMonitoring();
    }
    
    return 0;
}

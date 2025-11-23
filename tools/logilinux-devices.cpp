/*
 * logilinux-devices - List all connected Logitech devices
 * 
 * Usage:
 *   logilinux-devices [OPTIONS]
 * 
 * Options:
 *   --json         Output in JSON format (default: human-readable)
 *   --type TYPE    Filter by device type (dialpad, keypad)
 *   --help         Show this help message
 */

#include <logilinux/logilinux.h>
#include <logilinux/device.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

void printHelp(const char* progName) {
    std::cout << "Usage: " << progName << " [OPTIONS]\n\n"
              << "List all connected Logitech devices.\n\n"
              << "Options:\n"
              << "  --json         Output in JSON format (default: human-readable)\n"
              << "  --type TYPE    Filter by device type (dialpad, keypad)\n"
              << "  --help         Show this help message\n\n"
              << "Device Types:\n"
              << "  dialpad        Logitech MX Dialpad\n"
              << "  keypad         Logitech MX Creative Console / MX Keypad\n\n"
              << "Examples:\n"
              << "  " << progName << "                    # List all devices\n"
              << "  " << progName << " --json             # JSON output\n"
              << "  " << progName << " --type dialpad     # Only show dialpads\n"
              << "  " << progName << " --json | jq .      # Pretty JSON with jq\n";
}

const char* deviceTypeToString(LogiLinux::DeviceType type) {
    switch (type) {
        case LogiLinux::DeviceType::DIALPAD:
            return "dialpad";
        case LogiLinux::DeviceType::MX_KEYPAD:
            return "keypad";
        default:
            return "unknown";
    }
}

void printDevicesHuman(const std::vector<LogiLinux::DevicePtr>& devices) {
    if (devices.empty()) {
        std::cout << "No Logitech devices found." << std::endl;
        return;
    }

    std::cout << "Found " << devices.size() << " device(s):\n" << std::endl;

    for (const auto& device : devices) {
        const auto& info = device->getInfo();
        
        std::cout << "Device: " << info.name << std::endl;
        std::cout << "  Type:       " << deviceTypeToString(info.type) << std::endl;
        std::cout << "  Vendor ID:  0x" << std::hex << std::setw(4) << std::setfill('0') 
                  << info.vendor_id << std::dec << std::setfill(' ') << std::endl;
        std::cout << "  Product ID: 0x" << std::hex << std::setw(4) << std::setfill('0') 
                  << info.product_id << std::dec << std::setfill(' ') << std::endl;
        std::cout << "  Path:       " << info.device_path << std::endl;
        
        // List capabilities
        std::cout << "  Capabilities: ";
        std::vector<std::string> caps;
        if (device->hasCapability(LogiLinux::DeviceCapability::ROTATION))
            caps.push_back("rotation");
        if (device->hasCapability(LogiLinux::DeviceCapability::BUTTONS))
            caps.push_back("buttons");
        if (device->hasCapability(LogiLinux::DeviceCapability::HIGH_RES_SCROLL))
            caps.push_back("high-res-scroll");
        if (device->hasCapability(LogiLinux::DeviceCapability::LCD_DISPLAY))
            caps.push_back("lcd-display");
        if (device->hasCapability(LogiLinux::DeviceCapability::IMAGE_UPLOAD))
            caps.push_back("image-upload");
        
        for (size_t i = 0; i < caps.size(); i++) {
            std::cout << caps[i];
            if (i < caps.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl << std::endl;
    }
}

void printDevicesJSON(const std::vector<LogiLinux::DevicePtr>& devices) {
    std::cout << "{" << std::endl;
    std::cout << "  \"count\": " << devices.size() << "," << std::endl;
    std::cout << "  \"devices\": [" << std::endl;
    
    for (size_t i = 0; i < devices.size(); i++) {
        const auto& device = devices[i];
        const auto& info = device->getInfo();
        
        std::cout << "    {" << std::endl;
        std::cout << "      \"name\": \"" << info.name << "\"," << std::endl;
        std::cout << "      \"type\": \"" << deviceTypeToString(info.type) << "\"," << std::endl;
        std::cout << "      \"vendor_id\": \"0x" << std::hex << std::setw(4) << std::setfill('0') 
                  << info.vendor_id << "\"," << std::dec << std::setfill(' ') << std::endl;
        std::cout << "      \"product_id\": \"0x" << std::hex << std::setw(4) << std::setfill('0') 
                  << info.product_id << "\"," << std::dec << std::setfill(' ') << std::endl;
        std::cout << "      \"path\": \"" << info.device_path << "\"," << std::endl;
        
        // Capabilities array
        std::cout << "      \"capabilities\": [";
        std::vector<std::string> caps;
        if (device->hasCapability(LogiLinux::DeviceCapability::ROTATION))
            caps.push_back("\"rotation\"");
        if (device->hasCapability(LogiLinux::DeviceCapability::BUTTONS))
            caps.push_back("\"buttons\"");
        if (device->hasCapability(LogiLinux::DeviceCapability::HIGH_RES_SCROLL))
            caps.push_back("\"high-res-scroll\"");
        if (device->hasCapability(LogiLinux::DeviceCapability::LCD_DISPLAY))
            caps.push_back("\"lcd-display\"");
        if (device->hasCapability(LogiLinux::DeviceCapability::IMAGE_UPLOAD))
            caps.push_back("\"image-upload\"");
        
        for (size_t j = 0; j < caps.size(); j++) {
            std::cout << caps[j];
            if (j < caps.size() - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
        
        std::cout << "    }";
        if (i < devices.size() - 1) std::cout << ",";
        std::cout << std::endl;
    }
    
    std::cout << "  ]" << std::endl;
    std::cout << "}" << std::endl;
}

int main(int argc, char* argv[]) {
    bool jsonOutput = false;
    std::string filterType;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printHelp(argv[0]);
            return 0;
        } else if (arg == "--json") {
            jsonOutput = true;
        } else if (arg == "--type") {
            if (i + 1 < argc) {
                filterType = argv[++i];
            } else {
                std::cerr << "Error: --type requires an argument" << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            std::cerr << "Use --help for usage information." << std::endl;
            return 1;
        }
    }
    
    // Discover devices
    LogiLinux::Library lib;
    auto devices = lib.discoverDevices();
    
    // Filter by type if requested
    if (!filterType.empty()) {
        LogiLinux::DeviceType targetType;
        
        if (filterType == "dialpad") {
            targetType = LogiLinux::DeviceType::DIALPAD;
        } else if (filterType == "keypad") {
            targetType = LogiLinux::DeviceType::MX_KEYPAD;
        } else {
            std::cerr << "Error: Invalid device type: " << filterType << std::endl;
            std::cerr << "Valid types: dialpad, keypad" << std::endl;
            return 1;
        }
        
        std::vector<LogiLinux::DevicePtr> filtered;
        for (const auto& device : devices) {
            if (device->getType() == targetType) {
                filtered.push_back(device);
            }
        }
        devices = filtered;
    }
    
    // Output results
    if (jsonOutput) {
        printDevicesJSON(devices);
    } else {
        printDevicesHuman(devices);
    }
    
    return devices.empty() ? 1 : 0;
}

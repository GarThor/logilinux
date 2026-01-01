/*
 * keypad-set-image - Set JPEG image on MX Keypad LCD button
 * 
 * Usage:
 *   keypad-set-image [OPTIONS] <button> <image.jpg>
 *   echo <jpeg_data> | keypad-set-image [OPTIONS] <button> -
 * 
 * Options:
 *   --all                Set image on all buttons (0-8)
 *   --device PATH        Use specific device path
 *   --help               Show this help message
 */

#include <logilinux/logilinux.h>
#include <logilinux/device.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

// Need to include the implementation header for LCD functions
#include "../lib/src/devices/mx_keypad_device.h"

void printHelp(const char* progName) {
    std::cout << "Usage: " << progName << " [OPTIONS] <button> <image.jpg>\n"
              << "       echo <jpeg_data> | " << progName << " [OPTIONS] <button> -\n\n"
              << "Set JPEG image on MX Keypad LCD button.\n\n"
              << "Options:\n"
              << "  --all                Set image on all buttons (0-8)\n"
              << "  --device PATH        Use specific device path\n"
              << "  --help               Show this help message\n\n"
              << "Arguments:\n"
              << "  button               Button index (0-8) or name (GRID_0 to GRID_8)\n"
              << "  image.jpg            Path to JPEG image file (118x118 recommended)\n"
              << "                       Use '-' to read from stdin\n\n"
              << "Examples:\n"
              << "  " << progName << " 0 logo.jpg              # Set button 0\n"
              << "  " << progName << " GRID_5 icon.jpg         # Set button 5 by name\n"
              << "  " << progName << " --all background.jpg    # Set all buttons\n"
              << "  cat image.jpg | " << progName << " 3 -      # Read from stdin\n"
              << "  convert input.png -resize 118x118 - | " << progName << " 0 -\n\n"
              << "Note: Images should be 118x118 pixels. Larger images may be cropped.\n"
              << "      Requires sudo or appropriate permissions for hidraw access.\n";
}

int parseButtonIndex(const std::string& button) {
    // Try parsing as number first
    try {
        int index = std::stoi(button);
        if (index >= 0 && index <= 8) {
            return index;
        }
    } catch (...) {
        // Not a number, try name
    }
    
    // Try parsing as GRID_N
    if (button.find("GRID_") == 0 && button.length() == 6) {
        char lastChar = button[5];
        if (lastChar >= '0' && lastChar <= '8') {
            return lastChar - '0';
        }
    }
    
    return -1;
}

std::vector<uint8_t> readJpegFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    
    if (!file) {
        return {};
    }
    
    return data;
}

std::vector<uint8_t> readJpegStdin() {
    std::vector<uint8_t> data;
    
    char buffer[4096];
    while (std::cin.read(buffer, sizeof(buffer)) || std::cin.gcount() > 0) {
        data.insert(data.end(), buffer, buffer + std::cin.gcount());
    }
    
    return data;
}

int main(int argc, char* argv[]) {
    bool setAll = false;
    std::string devicePath;
    std::string buttonArg;
    std::string imagePath;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printHelp(argv[0]);
            return 0;
        } else if (arg == "--all") {
            setAll = true;
        } else if (arg == "--device") {
            if (i + 1 < argc) {
                devicePath = argv[++i];
            } else {
                std::cerr << "Error: --device requires an argument" << std::endl;
                return 1;
            }
        } else if (buttonArg.empty() && !setAll) {
            buttonArg = arg;
        } else if (imagePath.empty()) {
            imagePath = arg;
        } else {
            std::cerr << "Error: Too many arguments" << std::endl;
            std::cerr << "Use --help for usage information." << std::endl;
            return 1;
        }
    }
    
    // Validate arguments
    if (buttonArg.empty() && !setAll) {
        std::cerr << "Error: Missing required argument: button index" << std::endl;
        std::cerr << "Use --help for usage information." << std::endl;
        return 1;
    }
    else if (imagePath.empty()) {
        std::cerr << "Error: Missing required argument: image path" << std::endl;
        std::cerr << "Use --help for usage information." << std::endl;
        return 1;
    }

    int buttonIndex = parseButtonIndex(buttonArg);
    if (!setAll && buttonIndex < 0) {
        std::cerr << "Error: Invalid button index: " << buttonArg << std::endl;
        std::cerr << "Valid values: 0-8 or GRID_0 to GRID_8" << std::endl;
        return 1;
    }
    
    // Read JPEG data
    std::vector<uint8_t> jpegData;
    
    if (imagePath == "-") {
        jpegData = readJpegStdin();
        if (jpegData.empty()) {
            std::cerr << "Error: No data received from stdin" << std::endl;
            return 1;
        }
    } else {
        jpegData = readJpegFile(imagePath);
        if (jpegData.empty()) {
            std::cerr << "Error: Failed to read image file: " << imagePath << std::endl;
            return 1;
        }
    }
    
    // Verify JPEG header
    if (jpegData.size() < 2 || jpegData[0] != 0xff || jpegData[1] != 0xd8) {
        std::cerr << "Error: File does not appear to be a valid JPEG" << std::endl;
        return 1;
    }
    
    // Find device
    LogiLinux::Library lib;
    LogiLinux::MXKeypadDevice* keypad = nullptr;
    
    if (!devicePath.empty()) {
        auto devices = lib.discoverDevices();
        for (const auto& dev : devices) {
            if (dev->getType() == LogiLinux::DeviceType::MX_KEYPAD &&
                dev->getInfo().device_path == devicePath) {
                keypad = dynamic_cast<LogiLinux::MXKeypadDevice*>(dev.get());
                break;
            }
        }
        
        if (!keypad) {
            std::cerr << "Error: No MX Keypad found at " << devicePath << std::endl;
            return 1;
        }
    } else {
        auto device = lib.findDevice(LogiLinux::DeviceType::MX_KEYPAD);
        if (!device) {
            std::cerr << "Error: No MX Keypad found" << std::endl;
            std::cerr << "Make sure device is connected." << std::endl;
            return 1;
        }
        keypad = dynamic_cast<LogiLinux::MXKeypadDevice*>(device.get());
    }
    
    if (!keypad) {
        std::cerr << "Error: Device is not an MX Keypad" << std::endl;
        return 1;
    }
    
    // Check if device has LCD capability
    if (!keypad->hasCapability(LogiLinux::DeviceCapability::LCD_DISPLAY)) {
        std::cerr << "Error: Device does not have LCD display capability" << std::endl;
        return 1;
    }
    
    // Initialize device
    if (!keypad->initialize()) {
        std::cerr << "Error: Failed to initialize MX Keypad" << std::endl;
        std::cerr << "Try running with sudo for hidraw access." << std::endl;
        return 1;
    }
    
    // Set image
    if (setAll) {
        std::cout << "Setting image on all buttons..." << std::endl;
        for (int i = 0; i < 9; i++) {
            if (!keypad->setKeyImage(i, jpegData)) {
                std::cerr << "Error: Failed to set image on button " << i << std::endl;
                return 1;
            }
            std::cout << "  Button " << i << " done" << std::endl;
        }
        std::cout << "All buttons updated successfully" << std::endl;
    } else {
        if (!keypad->setKeyImage(buttonIndex, jpegData)) {
            std::cerr << "Error: Failed to set image on button " << buttonIndex << std::endl;
            return 1;
        }
        std::cout << "Image set successfully on button " << buttonIndex << std::endl;
    }
    
    return 0;
}

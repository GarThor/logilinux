/*
 * keypad-set-color - Set solid color on MX Keypad LCD button
 * 
 * Usage:
 *   keypad-set-color [OPTIONS] <button> <color>
 * 
 * Options:
 *   --all                Set color on all buttons (0-8)
 *   --device PATH        Use specific device path
 *   --help               Show this help message
 */

#include <logilinux/logilinux.h>
#include <logilinux/device.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

// Need to include the implementation header for LCD functions
#include "../lib/src/devices/mx_keypad_device.h"

void printHelp(const char* progName) {
    std::cout << "Usage: " << progName << " [OPTIONS] <button> <color>\n\n"
              << "Set solid color on MX Keypad LCD button.\n\n"
              << "Options:\n"
              << "  --all                Set color on all buttons (0-8)\n"
              << "  --device PATH        Use specific device path\n"
              << "  --help               Show this help message\n\n"
              << "Arguments:\n"
              << "  button               Button index (0-8) or name (GRID_0 to GRID_8)\n"
              << "  color                Color in format: RGB, #RRGGBB, or name\n"
              << "                       RGB format: r,g,b (0-255 each)\n"
              << "                       Hex format: #RRGGBB or RRGGBB\n"
              << "                       Names: red, green, blue, yellow, cyan, magenta,\n"
              << "                              white, black, orange, purple, pink, lime\n\n"
              << "Examples:\n"
              << "  " << progName << " 0 red                   # Set button 0 to red\n"
              << "  " << progName << " 5 #FF8000              # Set button 5 to orange\n"
              << "  " << progName << " GRID_3 255,128,0       # RGB format\n"
              << "  " << progName << " --all blue             # Set all buttons to blue\n"
              << "  " << progName << " 4 00FF00               # Green (hex without #)\n\n"
              << "Note: Requires ImageMagick 'convert' command to generate JPEG.\n"
              << "      Requires sudo or appropriate permissions for hidraw access.\n";
}

struct Color {
    uint8_t r, g, b;
};

int parseButtonIndex(const std::string& button) {
    try {
        int index = std::stoi(button);
        if (index >= 0 && index <= 8) {
            return index;
        }
    } catch (...) {}
    
    if (button.find("GRID_") == 0 && button.length() == 6) {
        char lastChar = button[5];
        if (lastChar >= '0' && lastChar <= '8') {
            return lastChar - '0';
        }
    }
    
    return -1;
}

bool parseColor(const std::string& colorStr, Color& color) {
    // Try named colors first
    std::string lower = colorStr;
    for (auto& c : lower) c = std::tolower(c);
    
    if (lower == "red") { color = {255, 0, 0}; return true; }
    if (lower == "green") { color = {0, 255, 0}; return true; }
    if (lower == "blue") { color = {0, 0, 255}; return true; }
    if (lower == "yellow") { color = {255, 255, 0}; return true; }
    if (lower == "cyan") { color = {0, 255, 255}; return true; }
    if (lower == "magenta") { color = {255, 0, 255}; return true; }
    if (lower == "white") { color = {255, 255, 255}; return true; }
    if (lower == "black") { color = {0, 0, 0}; return true; }
    if (lower == "orange") { color = {255, 128, 0}; return true; }
    if (lower == "purple") { color = {128, 0, 128}; return true; }
    if (lower == "pink") { color = {255, 192, 203}; return true; }
    if (lower == "lime") { color = {0, 255, 0}; return true; }
    
    // Try hex format (#RRGGBB or RRGGBB)
    std::string hex = colorStr;
    if (hex[0] == '#') {
        hex = hex.substr(1);
    }
    
    if (hex.length() == 6) {
        try {
            unsigned long val = std::stoul(hex, nullptr, 16);
            color.r = (val >> 16) & 0xFF;
            color.g = (val >> 8) & 0xFF;
            color.b = val & 0xFF;
            return true;
        } catch (...) {}
    }
    
    // Try RGB format (r,g,b)
    if (colorStr.find(',') != std::string::npos) {
        std::istringstream ss(colorStr);
        std::string r, g, b;
        
        if (std::getline(ss, r, ',') && std::getline(ss, g, ',') && std::getline(ss, b)) {
            try {
                int ri = std::stoi(r);
                int gi = std::stoi(g);
                int bi = std::stoi(b);
                
                if (ri >= 0 && ri <= 255 && gi >= 0 && gi <= 255 && bi >= 0 && bi <= 255) {
                    color.r = ri;
                    color.g = gi;
                    color.b = bi;
                    return true;
                }
            } catch (...) {}
        }
    }
    
    return false;
}

std::vector<uint8_t> generateColorJPEG(uint8_t r, uint8_t g, uint8_t b) {
    // Create temporary PPM file
    char ppmPath[] = "/tmp/logilinux_color_XXXXXX.ppm";
    char jpgPath[] = "/tmp/logilinux_color_XXXXXX.jpg";
    
    int ppmFd = mkstemps(ppmPath, 4);
    if (ppmFd < 0) {
        return {};
    }
    
    int jpgFd = mkstemps(jpgPath, 4);
    if (jpgFd < 0) {
        close(ppmFd);
        unlink(ppmPath);
        return {};
    }
    close(jpgFd);
    
    // Write PPM
    FILE* ppm = fdopen(ppmFd, "wb");
    if (!ppm) {
        close(ppmFd);
        unlink(ppmPath);
        unlink(jpgPath);
        return {};
    }
    
    fprintf(ppm, "P6\n118 118\n255\n");
    for (int i = 0; i < 118 * 118; i++) {
        uint8_t rgb[3] = {r, g, b};
        fwrite(rgb, 1, 3, ppm);
    }
    fclose(ppm);
    
    // Convert to JPEG using ImageMagick
    std::string cmd = "convert " + std::string(ppmPath) + " -quality 85 " + std::string(jpgPath) + " 2>/dev/null";
    int ret = system(cmd.c_str());
    
    unlink(ppmPath);
    
    if (ret != 0) {
        unlink(jpgPath);
        return {};
    }
    
    // Read JPEG
    FILE* jpg = fopen(jpgPath, "rb");
    if (!jpg) {
        unlink(jpgPath);
        return {};
    }
    
    fseek(jpg, 0, SEEK_END);
    long size = ftell(jpg);
    fseek(jpg, 0, SEEK_SET);
    
    std::vector<uint8_t> jpegData(size);
    fread(jpegData.data(), 1, size, jpg);
    fclose(jpg);
    
    unlink(jpgPath);
    
    return jpegData;
}

int main(int argc, char* argv[]) {
    bool setAll = false;
    std::string devicePath;
    std::string buttonArg;
    std::string colorArg;
    
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
        } else if (colorArg.empty()) {
            colorArg = arg;
        } else {
            std::cerr << "Error: Too many arguments" << std::endl;
            return 1;
        }
    }
    
    if (buttonArg.empty() && !setAll) {
        std::cerr << "Error: Missing required argument: button index" << std::endl;
        std::cerr << "Use --help for usage information." << std::endl;
        return 1;
    }
    else if (colorArg.empty()) {
        std::cerr << "Error: Missing required argument: color index" << std::endl;
        std::cerr << "Use --help for usage information." << std::endl;
        return 1;
    }
    
    int buttonIndex = parseButtonIndex(buttonArg);
    if (!setAll && buttonIndex < 0) {
        std::cerr << "Error: Invalid button index: " << buttonArg << std::endl;
        return 1;
    }
    
    Color color;
    if (!parseColor(colorArg, color)) {
        std::cerr << "Error: Invalid color format: " << colorArg << std::endl;
        std::cerr << "Use --help for color format information." << std::endl;
        return 1;
    }
    
    // Generate JPEG for this color
    auto jpegData = generateColorJPEG(color.r, color.g, color.b);
    if (jpegData.empty()) {
        std::cerr << "Error: Failed to generate color image" << std::endl;
        std::cerr << "Make sure ImageMagick 'convert' is installed." << std::endl;
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
            return 1;
        }
        keypad = dynamic_cast<LogiLinux::MXKeypadDevice*>(device.get());
    }
    
    if (!keypad || !keypad->hasCapability(LogiLinux::DeviceCapability::LCD_DISPLAY)) {
        std::cerr << "Error: Device does not have LCD display capability" << std::endl;
        return 1;
    }
    
    if (!keypad->initialize()) {
        std::cerr << "Error: Failed to initialize MX Keypad" << std::endl;
        std::cerr << "Try running with sudo." << std::endl;
        return 1;
    }
    
    // Set color
    if (setAll) {
        std::cout << "Setting color RGB(" << (int)color.r << "," << (int)color.g << "," << (int)color.b << ") on all buttons..." << std::endl;
        for (int i = 0; i < 9; i++) {
            if (!keypad->setKeyImage(i, jpegData)) {
                std::cerr << "Error: Failed to set color on button " << i << std::endl;
                return 1;
            }
            std::cout << "  Button " << i << " done" << std::endl;
        }
        std::cout << "All buttons updated successfully" << std::endl;
    } else {
        if (!keypad->setKeyImage(buttonIndex, jpegData)) {
            std::cerr << "Error: Failed to set color on button " << buttonIndex << std::endl;
            return 1;
        }
        std::cout << "Color RGB(" << (int)color.r << "," << (int)color.g << "," << (int)color.b 
                  << ") set successfully on button " << buttonIndex << std::endl;
    }
    
    return 0;
}

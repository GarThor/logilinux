/*
 * keypad-set-gif - Set animated GIF on MX Keypad LCD button
 * 
 * Usage:
 *   keypad-set-gif [OPTIONS] <button> <animation.gif>
 * 
 * Options:
 *   --all                Set GIF on all buttons (0-8)
 *   --no-loop            Don't loop animation (play once)
 *   --device PATH        Use specific device path
 *   --help               Show this help message
 */

#include <logilinux/logilinux.h>
#include <logilinux/device.h>
#include <iostream>
#include <string>
#include <atomic>
#include <csignal>
#include <thread>
#include <chrono>

// Need to include the implementation header for LCD functions
#include "../lib/src/devices/mx_keypad_device.h"

std::atomic<bool> running(true);

void signalHandler(int) {
    running = false;
}

void printHelp(const char* progName) {
    std::cout << "Usage: " << progName << " [OPTIONS] <button> <animation.gif>\n\n"
              << "Set animated GIF on MX Keypad LCD button.\n\n"
              << "Options:\n"
              << "  --all                Set GIF on all buttons (0-8)\n"
              << "  --no-loop            Don't loop animation (play once then stop)\n"
              << "  --device PATH        Use specific device path\n"
              << "  --help               Show this help message\n\n"
              << "Arguments:\n"
              << "  button               Button index (0-8) or name (GRID_0 to GRID_8)\n"
              << "  animation.gif        Path to GIF animation file\n\n"
              << "Examples:\n"
              << "  " << progName << " 0 spinner.gif          # Animate button 0\n"
              << "  " << progName << " GRID_5 loading.gif     # Animate button 5\n"
              << "  " << progName << " --all background.gif   # Animate all buttons\n"
              << "  " << progName << " --no-loop 3 intro.gif  # Play once on button 3\n\n"
              << "Note: GIF will be scaled to 118x118 pixels if needed.\n"
              << "      Animation runs until interrupted with Ctrl+C.\n"
              << "      Requires giflib and libjpeg for GIF support.\n"
              << "      Requires sudo or appropriate permissions for hidraw access.\n";
}

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

int main(int argc, char* argv[]) {
    bool setAll = false;
    bool loop = true;
    std::string devicePath;
    std::string buttonArg;
    std::string gifPath;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printHelp(argv[0]);
            return 0;
        } else if (arg == "--all") {
            setAll = true;
        } else if (arg == "--no-loop") {
            loop = false;
        } else if (arg == "--device") {
            if (i + 1 < argc) {
                devicePath = argv[++i];
            } else {
                std::cerr << "Error: --device requires an argument" << std::endl;
                return 1;
            }
        } else if (buttonArg.empty()) {
            buttonArg = arg;
        } else if (gifPath.empty()) {
            gifPath = arg;
        } else {
            std::cerr << "Error: Too many arguments" << std::endl;
            std::cerr << "Use --help for usage information." << std::endl;
            return 1;
        }
    }
    
    // Validate arguments
    if (buttonArg.empty() || gifPath.empty()) {
        std::cerr << "Error: Missing required arguments" << std::endl;
        std::cerr << "Use --help for usage information." << std::endl;
        return 1;
    }
    
    int buttonIndex = parseButtonIndex(buttonArg);
    if (!setAll && buttonIndex < 0) {
        std::cerr << "Error: Invalid button index: " << buttonArg << std::endl;
        std::cerr << "Valid values: 0-8 or GRID_0 to GRID_8" << std::endl;
        return 1;
    }
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
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
    
    // Set GIF animation
    if (setAll) {
        std::cout << "Loading GIF animation: " << gifPath << std::endl;
        std::cout << "Setting animation on all buttons..." << std::endl;
        
        for (int i = 0; i < 9; i++) {
            if (!keypad->setKeyGifFromFile(i, gifPath, loop)) {
                std::cerr << "Error: Failed to set GIF on button " << i << std::endl;
                std::cerr << "Make sure giflib and libjpeg are installed and the file is a valid GIF." << std::endl;
                return 1;
            }
            std::cout << "  Button " << i << " started" << std::endl;
        }
        std::cout << "All animations started" << std::endl;
    } else {
        std::cout << "Loading GIF animation: " << gifPath << std::endl;
        
        if (!keypad->setKeyGifFromFile(buttonIndex, gifPath, loop)) {
            std::cerr << "Error: Failed to set GIF on button " << buttonIndex << std::endl;
            std::cerr << "Make sure giflib and libjpeg are installed and the file is a valid GIF." << std::endl;
            return 1;
        }
        
        std::cout << "Animation started on button " << buttonIndex << std::endl;
    }
    
    if (loop) {
        std::cout << "\nAnimation is looping. Press Ctrl+C to stop." << std::endl;
    } else {
        std::cout << "\nAnimation will play once. Press Ctrl+C to stop early." << std::endl;
    }
    
    // Wait for interrupt
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Stop animations
    std::cout << "\nStopping animations..." << std::endl;
    keypad->stopAllAnimations();
    
    std::cout << "Done." << std::endl;
    
    return 0;
}

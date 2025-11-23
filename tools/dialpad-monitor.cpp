/*
 * dialpad-monitor - Monitor MX Dialpad events
 * 
 * Usage:
 *   dialpad-monitor [OPTIONS]
 * 
 * Options:
 *   --json               Output events in JSON format (one per line)
 *   --rotation-only      Only output rotation events
 *   --buttons-only       Only output button events
 *   --grab               Grab device exclusively (disable default behavior)
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
    bool rotationOnly = false;
    bool buttonsOnly = false;
    bool grab = false;
    std::string devicePath;
};

void printHelp(const char* progName) {
    std::cout << "Usage: " << progName << " [OPTIONS]\n\n"
              << "Monitor events from Logitech MX Dialpad.\n\n"
              << "Options:\n"
              << "  --json               Output events in JSON format (one per line)\n"
              << "  --rotation-only      Only output rotation events\n"
              << "  --buttons-only       Only output button events\n"
              << "  --grab               Grab device exclusively (disable default behavior)\n"
              << "  --device PATH        Use specific device path (e.g., /dev/input/event5)\n"
              << "  --help               Show this help message\n\n"
              << "Output Format (JSON):\n"
              << "  {\"type\":\"rotation\",\"delta\":1,\"delta_high_res\":120,\"timestamp\":1234567}\n"
              << "  {\"type\":\"button\",\"action\":\"press\",\"button\":\"TOP_LEFT\",\"code\":275,\"timestamp\":1234567}\n\n"
              << "Examples:\n"
              << "  " << progName << "                     # Monitor all events (human-readable)\n"
              << "  " << progName << " --json              # JSON output for scripting\n"
              << "  " << progName << " --rotation-only     # Only rotation events\n"
              << "  " << progName << " --grab              # Exclusive grab\n"
              << "  " << progName << " --json | jq .       # Pretty JSON with jq\n\n"
              << "Pipe to other commands:\n"
              << "  " << progName << " --json --rotation-only | while read event; do\n"
              << "    delta=$(echo $event | jq -r .delta)\n"
              << "    # Process delta...\n"
              << "  done\n";
}

void handleEvent(LogiLinux::EventPtr event, const Options& opts) {
    if (auto rotation = std::dynamic_pointer_cast<LogiLinux::RotationEvent>(event)) {
        if (opts.buttonsOnly) return;
        
        if (opts.json) {
            std::cout << "{\"type\":\"rotation\""
                     << ",\"delta\":" << rotation->delta
                     << ",\"delta_high_res\":" << rotation->delta_high_res
                     << ",\"timestamp\":" << rotation->timestamp
                     << "}" << std::endl;
        } else {
            std::cout << "[ROTATION] Delta: " << std::setw(3) << rotation->delta
                     << " | High-res: " << std::setw(5) << rotation->delta_high_res
                     << " | Timestamp: " << rotation->timestamp << std::endl;
        }
    } 
    else if (auto button = std::dynamic_pointer_cast<LogiLinux::ButtonEvent>(event)) {
        if (opts.rotationOnly) return;
        
        auto dialpadButton = LogiLinux::getDialpadButton(button->button_code);
        const char* buttonName = LogiLinux::getDialpadButtonName(dialpadButton);
        
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
                     << " | Code: " << button->button_code
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
        } else if (arg == "--rotation-only") {
            opts.rotationOnly = true;
        } else if (arg == "--buttons-only") {
            opts.buttonsOnly = true;
        } else if (arg == "--grab") {
            opts.grab = true;
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
    if (opts.rotationOnly && opts.buttonsOnly) {
        std::cerr << "Error: Cannot use --rotation-only and --buttons-only together" << std::endl;
        return 1;
    }
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    LogiLinux::Library lib;
    LogiLinux::DevicePtr dialpad;
    
    // Find device
    if (!opts.devicePath.empty()) {
        // TODO: Library doesn't support finding by path yet
        // For now, find any dialpad and check if path matches
        auto devices = lib.discoverDevices();
        for (const auto& dev : devices) {
            if (dev->getType() == LogiLinux::DeviceType::DIALPAD &&
                dev->getInfo().device_path == opts.devicePath) {
                dialpad = dev;
                break;
            }
        }
        
        if (!dialpad) {
            std::cerr << "Error: No dialpad found at " << opts.devicePath << std::endl;
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
    
    // Set up event callback
    dialpad->setEventCallback([&opts](LogiLinux::EventPtr event) {
        handleEvent(event, opts);
    });
    
    // Grab if requested
    if (opts.grab) {
        if (!dialpad->grabExclusive(true)) {
            std::cerr << "Warning: Failed to grab device exclusively" << std::endl;
            std::cerr << "Try running with sudo for exclusive access." << std::endl;
        }
    }
    
    // Start monitoring
    dialpad->startMonitoring();
    
    if (!dialpad->isMonitoring()) {
        std::cerr << "Error: Failed to start monitoring" << std::endl;
        std::cerr << "Try running with sudo if you get permission errors." << std::endl;
        return 1;
    }
    
    // Wait for events
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Cleanup
    dialpad->stopMonitoring();
    
    return 0;
}

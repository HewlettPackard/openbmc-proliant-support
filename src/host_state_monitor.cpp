#include <iostream>
#include <string>
#include <memory>
#include <signal.h>
#include <unistd.h>
#include <chrono>
#include <map>
#include <vector>
#include <variant>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <sys/mman.h>
#include <fcntl.h>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/exception.hpp>

class HostStateMonitor {
private:
    sdbusplus::bus::bus bus;
    std::unique_ptr<sdbusplus::bus::match::match> match;
    bool running;
    std::string previousState;

    static const std::string BUS_NAME;
    static const std::string OBJECT_PATH;
    static const std::string INTERFACE_NAME;
    static const std::string PROPERTY_NAME;

public:
    HostStateMonitor() : bus(sdbusplus::bus::new_default_system()), running(true), previousState("") {}

    ~HostStateMonitor() = default;

    int initialize() {
        try {
            // Get initial state
            getHostState();

            // Subscribe to property changes using sdbusplus match rules
            std::string matchRule = sdbusplus::bus::match::rules::propertiesChanged(
                OBJECT_PATH, INTERFACE_NAME);

            match = std::make_unique<sdbusplus::bus::match::match>(
                bus,
                matchRule,
                [this](sdbusplus::message::message& msg) {
                    onPropertiesChanged(msg);
                }
            );

            return 0;
        } catch (const sdbusplus::exception::exception& e) {
            std::cerr << "Failed to initialize: " << e.what() << std::endl;
            return -1;
        }
    }

    void run() {
        std::cout << "Monitoring host state changes. Press Ctrl+C to exit..." << std::endl;
        
        while (running) {
            try {
                bus.process_discard();
                bus.wait(std::chrono::milliseconds(1000));
            } catch (const sdbusplus::exception::exception& e) {
                std::cerr << "Error processing bus: " << e.what() << std::endl;
                break;
            }
        }
    }

    void stop() {
        running = false;
    }

private:
    void getHostState() {
        try {
            auto method = bus.new_method_call(
                BUS_NAME.c_str(),
                OBJECT_PATH.c_str(),
                "org.freedesktop.DBus.Properties",
                "Get"
            );
            method.append(INTERFACE_NAME, PROPERTY_NAME);

            auto reply = bus.call(method);
            std::variant<std::string> value;
            reply.read(value);

            std::string state = std::get<std::string>(value);
            handleHostState(state.c_str());
        } catch (const sdbusplus::exception::exception& e) {
            std::cerr << "Failed to get host state: " << e.what() << std::endl;
        }
    }

    void onPropertiesChanged(sdbusplus::message::message& msg) {
        try {
            std::string interface;
            std::map<std::string, std::variant<std::string>> changedProperties;
            std::vector<std::string> invalidatedProperties;

            msg.read(interface, changedProperties, invalidatedProperties);

            if (interface != INTERFACE_NAME) {
                return;
            }

            auto it = changedProperties.find(PROPERTY_NAME);
            if (it != changedProperties.end()) {
                std::string state = std::get<std::string>(it->second);
                handleHostState(state.c_str());
            }
        } catch (const sdbusplus::exception::exception& e) {
            std::cerr << "Error processing properties changed: " << e.what() << std::endl;
        }
    }

    void handleHostState(const char* state) {
        std::string stateStr(state);
        std::cout << "Host State Changed: ";
        
        bool wasOn = (previousState.find("Running") != std::string::npos ||
                     previousState.find("On") != std::string::npos);
        bool isOff = (stateStr.find("Off") != std::string::npos ||
                     stateStr.find("Quiesced") != std::string::npos ||
                     stateStr.find("TransitioningToOff") != std::string::npos);
        
        if (stateStr.find("Running") != std::string::npos ||
            stateStr.find("On") != std::string::npos) {
            std::cout << "ON" << std::endl;
        } else if (isOff) {
            std::cout << "OFF" << std::endl;
            // Execute shutdown procedures only when transitioning from ON to OFF
            if (wasOn && !previousState.empty()) {
                std::cout << "Detected transition from ON to OFF - executing shutdown procedures" << std::endl;
                handleShutdown();
            }
        } else {
            std::cout << state << " (Unknown state)" << std::endl;
        }
        
        // Update previous state for next comparison
        previousState = stateStr;
    }

    void handleShutdown() {
        std::cout << "Executing shutdown procedures..." << std::endl;
        
        try {
            // Read shutdown reason from memory address 0x80000074
            uint16_t shutdownReason = readMemory16(0x80000074);
            
            // Print shutdown reason
            std::cout << "Shutdown reason: " << shutdownReason << std::endl;
            
            // Calculate isSet2 (shutdownReason | 0xFBFF)
            uint16_t isSet2 = shutdownReason | 0xFBFF;
            
            // Get restart cause via D-Bus
            std::string restartCause = getRestartCause();
            std::cout << "Restart cause: " << restartCause << std::endl;
            
            // Check restart conditions
            if (isSet2 == 65535 || restartCause == "xyz.openbmc_project.State.Host.RestartCause.Unknown") {
                std::cout << "Restarting host..." << std::endl;
                
                // Log which condition triggered the restart
                if (isSet2 == 65535) {
                    std::cout << "Restart triggered by: isSet2 condition (value: " << isSet2 << ")" << std::endl;
                }
                if (restartCause == "xyz.openbmc_project.State.Host.RestartCause.Unknown") {
                    std::cout << "Restart triggered by: unknown restart cause" << std::endl;
                }
                
                // Request host transition to On
                requestHostTransition("xyz.openbmc_project.State.Host.Transition.On");
            }
            
            std::cout << "Shutdown procedures completed successfully" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "Error during shutdown procedures: " << e.what() << std::endl;
        }
    }
    
    // Helper methods for shutdown procedures
    uint16_t readMemory16(uint32_t address) {
        int fd = open("/dev/mem", O_RDONLY | O_SYNC);
        if (fd < 0) {
            throw std::runtime_error("Failed to open /dev/mem");
        }
        
        void* map = mmap(nullptr, 4096, PROT_READ, MAP_SHARED, fd, address & ~0xFFF);
        close(fd);
        
        if (map == MAP_FAILED) {
            throw std::runtime_error("Failed to map memory");
        }
        
        uint16_t value = *((volatile uint16_t*)((char*)map + (address & 0xFFF)));
        munmap(map, 4096);
        
        return value;
    }
    
    std::string getRestartCause() {
        try {
            auto method = bus.new_method_call(
                "xyz.openbmc_project.State.Host",
                "/xyz/openbmc_project/control/host0/restart_cause",
                "org.freedesktop.DBus.Properties",
                "Get"
            );
            method.append("xyz.openbmc_project.Control.Host.RestartCause", "RestartCause");
            
            auto reply = bus.call(method);
            std::variant<std::string> value;
            reply.read(value);
            
            return std::get<std::string>(value);
        } catch (const sdbusplus::exception::exception& e) {
            std::cerr << "Failed to get restart cause: " << e.what() << std::endl;
            return "xyz.openbmc_project.State.Host.RestartCause.Unknown";
        }
    }
    
    void requestHostTransition(const std::string& transition) {
        try {
            auto method = bus.new_method_call(
                "xyz.openbmc_project.State.Host",
                "/xyz/openbmc_project/state/host0",
                "org.freedesktop.DBus.Properties",
                "Set"
            );
            method.append("xyz.openbmc_project.State.Host", "RequestedHostTransition");
            method.append(std::variant<std::string>(transition));
            
            bus.call(method);
            std::cout << "Set host transition to: " << transition << std::endl;
        } catch (const sdbusplus::exception::exception& e) {
            std::cerr << "Failed to set host transition: " << e.what() << std::endl;
        }
    }
};

// Static member definitions
const std::string HostStateMonitor::BUS_NAME = "xyz.openbmc_project.State.Host";
const std::string HostStateMonitor::OBJECT_PATH = "/xyz/openbmc_project/state/host0";
const std::string HostStateMonitor::INTERFACE_NAME = "xyz.openbmc_project.State.Host";
const std::string HostStateMonitor::PROPERTY_NAME = "CurrentHostState";

// Global monitor instance for signal handling
HostStateMonitor* g_monitor = nullptr;

void signalHandler(int signal) {
    if (g_monitor) {
        std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
        g_monitor->stop();
    }
}

int main() {
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    HostStateMonitor monitor;
    g_monitor = &monitor;

    int ret = monitor.initialize();
    if (ret < 0) {
        std::cerr << "Failed to initialize monitor" << std::endl;
        return 1;
    }

    monitor.run();

    std::cout << "Host state monitor stopped." << std::endl;
    return 0;
}

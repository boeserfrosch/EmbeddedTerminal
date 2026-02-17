/*
 * DualNetworkTerminal Example
 *
 * This example demonstrates how to register the same command type with
 * different keywords for different network interfaces (e.g., Ethernet and WiFi).
 *
 * Commands available:
 * - help      : List all available commands
 * - eth-ip    : Show Ethernet network interface info
 * - wifi-ip   : Show WiFi network interface info
 * - ls, cd, cat, etc. : File system commands
 *
 * Usage:
 * 1. Upload this sketch to your board with dual network capabilities
 * 2. Open Serial Monitor (115200 baud)
 * 3. Try commands:
 *    > eth-ip
 *    > wifi-ip
 *    > help
 */

#include <Arduino.h>
#include <Terminal.h>
#include <DirectoryNavigator.h>
#include <commands/ip.h>
#include <BuiltinCommandFactory.h>
#include <interfaces/INetworkInterface.h>

// Use appropriate file system for your platform
#if defined(ESP32)
#include <hal/SDMMCFileSystem.h>
SDMMCFileSystem fileSystem;
#else
#include <hal/ArduinoFileSystem.h>
ArduinoFileSystem fileSystem;
#endif

// Mock network interfaces for demonstration
#include "../test/Mocks/MockNetworkInterface.h"

using namespace EmbeddedTerminal;

// Create terminal instance
Terminal term(Serial);

// Create factory instance (owns built-in commands)
EmbeddedTerminal::BuiltinCommandFactory factory;

// Directory navigator for file commands
DirectoryNavigator nav(&fileSystem);

// Create separate network interface instances
MockNetworkInterface ethernetInterface;
MockNetworkInterface wifiInterface;

// Create separate IP command instances for each network
cmd::ip ethIpCommand(ethernetInterface);
cmd::ip wifiIpCommand(wifiInterface);

void setup()
{
    // Initialize Serial communication
    Serial.begin(115200);
    delay(1000);

    // Welcome message
    Serial.println("\n\n");
    Serial.println("==========================================");
    Serial.println("  Dual Network Terminal Example");
    Serial.println("==========================================");
    Serial.println();

// Initialize file system
#if defined(ESP32)
    if (!fileSystem.begin())
    {
        Serial.println("ERROR: Failed to mount file system!");
    }
    else
    {
        Serial.println("File system initialized");
    }
#endif

    // Configure mock network interfaces with example data
    // In a real application, these would be actual network interfaces
    ethernetInterface.addInterface(
        "eth0",              // name
        "192.168.1.100",     // ip
        "00:11:22:33:44:55", // mac
        "255.255.255.0",     // netmask
        "192.168.1.1",       // gateway
        true                 // isUp
    );

    wifiInterface.addInterface(
        "wlan0",             // name
        "192.168.2.200",     // ip
        "AA:BB:CC:DD:EE:FF", // mac
        "255.255.255.0",     // netmask
        "192.168.2.1",       // gateway
        true                 // isUp
    );

    // Register filesystem and disk commands using factory
    factory.registerFilesystemCommands(term, nav, CMD_LS | CMD_CD | CMD_CAT);
    factory.registerDiskCommands(term, nav);
    factory.registerHelpCommand(term);

    // Register SEPARATE IP commands with DIFFERENT keywords
    // This demonstrates registering the same command type with different instances
    term.registerCommand("eth-ip", &ethIpCommand);
    term.registerCommand("wifi-ip", &wifiIpCommand);

    // Alternative approach: register under generic "ip" keyword with one interface
    // term.registerCommand("ip", &ethIpCommand);

    // Show usage
    Serial.println("Dual network interface configuration:");
    Serial.println("  - Ethernet: eth-ip");
    Serial.println("  - WiFi:     wifi-ip");
    Serial.println();
    Serial.println("File commands: ls, cd, cat, df");
    Serial.println("Type 'help' to see all available commands");
    Serial.println();
    Serial.println("Example usage:");
    Serial.println("  > eth-ip        # Show Ethernet interface info");
    Serial.println("  > wifi-ip       # Show WiFi interface info");
    Serial.println("  > ls            # List files");
    Serial.print("\n> ");
}

void loop()
{
    // Process terminal input
    term.loop();
}

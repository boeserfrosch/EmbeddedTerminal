/*
 * NetworkTerminal Example
 *
 * This example demonstrates how to create a terminal with network support
 * on ESP32. It includes the 'ip' command to display network interface
 * information (WiFi and/or Ethernet).
 *
 * Commands available:
 * - help    : List all available commands
 * - ls      : List files in current directory
 * - cd      : Change directory
 * - cat     : Display file contents
 * - mkdir   : Create a directory
 * - rm      : Remove a file
 * - rmdir   : Remove a directory
 * - tail    : Show end of file
 * - download: Download file via network
 * - df      : Show disk usage
 * - ip      : Show network interfaces
 *
 * Usage:
 * 1. Update WiFi credentials below (SSID and password)
 * 2. Upload this sketch to your ESP32 board
 * 3. Open Serial Monitor (115200 baud)
 * 4. Type commands and press Enter
 *
 * Example session:
 *   > help
 *   > ip
 *   > ls
 *   > df
 */

#include <Arduino.h>
#include <Terminal.h>
#include <BuiltinCommandFactory.h>
#include <DirectoryNavigator.h>

// ESP32-specific includes
#if defined(ESP32)
#include <WiFi.h>
#include <hal/SDMMCFileSystem.h>
#include <hal/ESPNetworkInterface.h>

// WiFi credentials - UPDATE THESE!
const char *ssid = "SSID";
const char *password = "password";

SDMMCFileSystem fileSystem;
ESPNetworkInterface networkInterface;
#else
#error "This example is designed for ESP32 only"
#endif

using namespace EmbeddedTerminal;

// Create instances
DirectoryNavigator nav(&fileSystem);
Terminal term(Serial);
BuiltinCommandFactory factory;

void setup()
{
    // Initialize Serial communication
    Serial.begin(115200);
    delay(1000); // Wait for Serial to initialize

    // Clear screen and show welcome message
    Serial.println("\n\n");
    Serial.println("=======================================");
    Serial.println("  Network Terminal Example (ESP32)");
    Serial.println("=======================================");
    Serial.println();

    // Connect to WiFi
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println();
        Serial.println("WiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println();
        Serial.println("WARNING: WiFi connection failed!");
        Serial.println("Network commands may not work properly.");
    }
    Serial.println();

    // Register all commands including network
    factory.registerAllCommands(term, nav, &networkInterface);

    // Show available commands
    Serial.println("Terminal ready! Type 'help' for available commands.");
    Serial.println("Try 'ip' to see network interface information.");
    Serial.println("Current directory: " + nav.pwd());
    Serial.print("\n> ");
}

void loop()
{
    // Process terminal input
    term.loop();

    // Add any other application logic here
    // The terminal runs non-blocking, so you can do other tasks
}

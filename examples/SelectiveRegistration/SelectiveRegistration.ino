/*
 * SelectiveRegistration Example
 *
 * This example demonstrates how to register only specific built-in commands
 * using command flags. This is useful when you want to save memory by only
 * including the commands you need.
 *
 * Commands registered in this example:
 * - ls    : List directory contents
 * - cd    : Change directory
 * - cat   : Display file contents
 * - df    : Show disk usage
 * - help  : Display available commands
 *
 * Usage:
 * 1. Upload this sketch to your ESP32/Arduino board
 * 2. Open Serial Monitor (115200 baud)
 * 3. Type commands and press Enter
 *
 * Example session:
 *   > help
 *   > ls
 *   > cd test
 *   > df
 */

#include <Arduino.h>
#include <Terminal.h>
#include <BuiltinCommandFactory.h>
#include <BuiltinCommandFlags.h>
#include <DirectoryNavigator.h>

// Platform-specific file system
#if defined(ESP32)
#include <hal/SDMMCFileSystem.h>
SDMMCFileSystem fileSystem;
#elif defined(ARDUINO)
#include <hal/ArduinoFileSystem.h>
ArduinoFileSystem fileSystem;
#else
#include <hal/NativeFileSystem.h>
NativeFileSystem fileSystem;
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
    Serial.println("=============================================");
    Serial.println("  Selective Command Registration Example");
    Serial.println("=============================================");
    Serial.println();

// Initialize file system (platform-specific)
#if defined(ESP32)
    if (!fileSystem.begin())
    {
        Serial.println("ERROR: Failed to mount file system!");
        Serial.println("Make sure SD card is inserted.");
    }
    else
    {
        Serial.println("File system initialized successfully");
    }
#endif

    // Register only filesystem navigation commands
    factory.registerFilesystemCommands(term, nav, CMD_LS | CMD_CD | CMD_CAT);

    // Register disk usage command
    factory.registerDiskCommands(term, nav);

    // Register network commands (requires INetworkInterface)
    // factory.registerNetworkCommands(term, networkInterface);

    // Register help command
    factory.registerHelpCommand(term);

    // Show available commands
    Serial.println();
    Serial.println("Registered commands: ls, cd, cat, df, help");
    Serial.println("Type 'help' to see available commands");
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

/*
 * BasicTerminal Example
 *
 * This example demonstrates basic usage of the EmbeddedTerminal library
 * on ESP32 or Arduino platforms. It creates a terminal with several
 * built-in commands accessible via Serial.
 *
 * Commands available:
 * - help : List all available commands
 * - ls   : List files in current directory
 * - cd   : Change directory
 * - cat  : Display file contents
 * - mkdir: Create a directory
 * - rm   : Remove a file
 *
 * Usage:
 * 1. Upload this sketch to your ESP32/Arduino board
 * 2. Open Serial Monitor (115200 baud)
 * 3. Type commands and press Enter
 *
 * Example session:
 *   > help
 *   > ls
 *   > mkdir test
 *   > cd test
 *   > pwd
 */

#include <Arduino.h>
#include <Terminal.h>
#include <commands/help.h>
#include <commands/ls.h>
#include <commands/cd.h>
#include <commands/cat.h>
#include <commands/mkdir.h>
#include <commands/rm.h>
#include <commands/df.h>
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

// Create terminal instance (Terminal accepts Stream directly on Arduino)
Terminal term(Serial);

// Directory navigator for file commands
DirectoryNavigator nav(&fileSystem);

// Command instances (must be static to persist)
static cmd::help helpCmd(term);
static cmd::ls lsCmd(nav);
static cmd::cd cdCmd(nav);
static cmd::cat catCmd(nav);
static cmd::mkdir mkdirCmd(nav);
static cmd::rm rmCmd(nav);
static cmd::df dfCmd(fileSystem);

void setup()
{
    // Initialize Serial communication
    Serial.begin(115200);
    delay(1000); // Wait for Serial to initialize

    // Clear screen and show welcome message
    Serial.println("\n\n");
    Serial.println("===================================");
    Serial.println("  EmbeddedTerminal - Basic Example");
    Serial.println("===================================");
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

    // Register commands with terminal
    term.registerCommand("help", &helpCmd);
    term.registerCommand("ls", &lsCmd);
    term.registerCommand("cd", &cdCmd);
    term.registerCommand("cat", &catCmd);
    term.registerCommand("mkdir", &mkdirCmd);
    term.registerCommand("rm", &rmCmd);
    term.registerCommand("df", &dfCmd);

    // Show available commands
    Serial.println();
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

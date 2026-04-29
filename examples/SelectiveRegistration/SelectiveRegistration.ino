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
#include <DirectoryNavigator.h>
#include <StorageSystem.h>
#include <commands/BuiltinCommands.h>
#include <interfaces/IStorage.h>
#include <hal/arduino/ArduinoFileSystem.h>

// Platform-specific file system
#if defined(ESP32)
#include <SPIFFS.h>
EmbeddedTerminal::ArduinoFileSystem fileSystem(SPIFFS);
#elif defined(ARDUINO)
#include <SD.h>
EmbeddedTerminal::ArduinoFileSystem fileSystem(SD);
#else
#include <hal/native/NativeFileSystem.h>
EmbeddedTerminal::NativeFileSystem fileSystem;
#endif

using namespace EmbeddedTerminal;

// Create instances
class ExampleStorageMedia : public IStorageMedia
{
public:
    ExampleStorageMedia(const ETString &name, IFileSystem *fs) : name_(name), fs_(fs) {}
    const char *name() const override { return name_.c_str(); }
    IFileSystem *fileSystem() override { return fs_; }
    bool isAvailable() const override { return true; }
    unsigned long long totalBytes() const override { return 0; }
    unsigned long long usedBytes() const override { return 0; }
    unsigned long long capacity() const override { return 0; }
    unsigned long long freeBytes() const override { return 0; }

private:
    ETString name_;
    IFileSystem *fs_;
};

StorageSystem storage;
ExampleStorageMedia media("default", &fileSystem);
DirectoryNavigator nav(&storage);
Terminal term(Serial);

cmd::cat catCommand(nav);
cmd::cd cdCommand(nav);
cmd::ls lsCommand(nav);
cmd::df dfCommand(storage);
cmd::help helpCommand(term);

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
    if (!SPIFFS.begin(true))
    {
        Serial.println("ERROR: Failed to mount file system!");
        Serial.println("SPIFFS mount failed.");
    }
    else
    {
        Serial.println("File system initialized successfully");
    }
#elif defined(ARDUINO)
    SD.begin();
#endif

    storage.mountMedia(&media, "");

    term.registerCommand("cat", &catCommand);
    term.registerCommand("cd", &cdCommand);
    term.registerCommand("ls", &lsCommand);
    term.registerCommand("df", &dfCommand);
    term.registerCommand("help", &helpCommand);

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

/*
 * SimpleCustomCommand Example
 *
 * This example demonstrates the simplest way to create a custom command
 * and register it with the terminal. This example creates a single
 * custom command called "mycmd" that responds with a greeting.
 *
 * Commands available:
 * - mycmd : Custom command that responds with greeting
 *
 * Usage:
 * 1. Upload this sketch to your ESP32/Arduino board
 * 2. Open Serial Monitor (115200 baud)
 * 3. Type commands and press Enter
 *
 * Example session:
 *   > mycmd
 *   Hello from custom command!
 *   > mycmd test
 *   Hello from custom command!
 */

#include <Arduino.h>
#include <Terminal.h>
#include <interfaces/ICommand.h>
#include <DirectoryNavigator.h>
#include <StorageSystem.h>
#include <interfaces/IStorage.h>
#include <hal/ArduinoFileSystem.h>

// Platform-specific file system
#if defined(ESP32)
#include <SPIFFS.h>
ArduinoFileSystem fs(SPIFFS);
#elif defined(ARDUINO)
#include <SD.h>
ArduinoFileSystem fs(SD);
#else
#include <hal/NativeFileSystem.h>
NativeFileSystem fs;
#endif

using namespace EmbeddedTerminal;

// Define a simple custom command
class MyCommand : public ICommand
{
public:
    ETString trigger(const ETString &keyword, const ETString &additional) override
    {
        return "Hello from custom command!";
    }

    ETString usage(const ETString &keyword) override
    {
        return "mycmd - Custom command example";
    }
};

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
ExampleStorageMedia media("default", &fs);
DirectoryNavigator nav(&storage);
MyCommand myCmd;
Terminal term(Serial);

void setup()
{
    // Initialize Serial communication
    Serial.begin(115200);
    delay(1000); // Wait for Serial to initialize

    // Clear screen and show welcome message
    Serial.println("\n\n");
    Serial.println("========================================");
    Serial.println("  Simple Custom Command Example");
    Serial.println("========================================");
    Serial.println();

#if defined(ESP32)
    SPIFFS.begin(true);
#elif defined(ARDUINO)
    SD.begin();
#endif

    storage.mountMedia(&media, "");

    // Register custom command
    term.registerCommand("mycmd", &myCmd);

    Serial.println("Terminal ready! Type 'help' for available commands.");
    Serial.println("Try the custom command: mycmd");
    Serial.print("\n> ");
}

void loop()
{
    // Process terminal input
    term.loop();

    // Add any other application logic here
    // The terminal runs non-blocking, so you can do other tasks
}

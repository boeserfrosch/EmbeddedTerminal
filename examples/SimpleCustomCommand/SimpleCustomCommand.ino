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
#include <hal/common/DefaultStorageMedia.h>

// Platform-specific file system
#if defined(ARDUINO) && defined(ESP32)
ArduinoSDMMCStorageMedia media;
#elif defined(ESP_PLATFORM) || defined(ESP_32)
ESPIDFSDMMCStorageMedia media("sdmmc", "/sdcard");
#else
NativeSuggestedStorageMedia media("native", ".");
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

StorageSystem storage;
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
    media.begin();
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

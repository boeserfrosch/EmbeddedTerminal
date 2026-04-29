/*
 * OwnershipExample
 *
 * This example demonstrates the clear ownership model:
 * - BuiltinCommandFactory OWNS the built-in commands it creates
 * - User OWNS any custom commands they create
 * - Terminal OWNS nothing - it just holds pointers
 *
 * This makes the lifetime management clear and consistent.
 */

#include <Arduino.h>
#include <Terminal.h>
#include <DirectoryNavigator.h>
#include <StorageSystem.h>
#include <commands/BuiltinCommands.h>
#include <interfaces/ICommand.h>
#include <interfaces/IStorage.h>
#include <hal/arduino/ArduinoFileSystem.h>

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

// Custom command - YOU own this
class RebootCommand : public ICommand
{
public:
    ETString trigger(const ETString &keyword, const ETString &additional) override
    {
#if defined(ESP32)
        ESP.restart();
#endif
        return "Rebooting...\n";
    }

    ETString usage(const ETString &keyword) override
    {
        return keyword + " - Reboot the system\n";
    }
};

// Terminal instance
Terminal term(Serial);

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

// Custom command - YOU own this and must keep it alive while registered
RebootCommand rebootCmd;

cmd::cat catCommand(nav);
cmd::cd cdCommand(nav);
cmd::ls lsCommand(nav);
cmd::help helpCommand(term);

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n");
    Serial.println("=================================");
    Serial.println("  Ownership Model Example");
    Serial.println("=================================");
    Serial.println();

#if defined(ESP32)
    SPIFFS.begin(true);
#elif defined(ARDUINO)
    SD.begin();
#endif

    storage.mountMedia(&media, "");

    term.registerCommand("cat", &catCommand);
    term.registerCommand("cd", &cdCommand);
    term.registerCommand("ls", &lsCommand);
    term.registerCommand("help", &helpCommand);
    term.registerCommand("reboot", &rebootCmd);

    Serial.println("Ownership model:");
    Serial.println("  - Built-in commands (ls, cd, cat, help): Application owns");
    Serial.println("  - Custom command (reboot): You own");
    Serial.println("  - Terminal: Owns nothing, just holds pointers");
    Serial.println();
    Serial.println("Try: help, ls, reboot");
    Serial.print("\n> ");
}

void loop()
{
    term.loop();
}

// When this goes out of scope:
// 1. factory destructor deletes all built-in commands
// 2. rebootCmd destructor called (no dynamic memory to clean up)
// 3. term destructor called (doesn't delete anything)
// Clear ownership - no leaks, no confusion!

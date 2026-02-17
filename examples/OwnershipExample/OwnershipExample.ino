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
#include <BuiltinCommandFactory.h>
#include <interfaces/ICommand.h>

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

// Factory instance - OWNS built-in commands (help, ls, cd, etc.)
// When factory goes out of scope, it deletes all built-in commands
BuiltinCommandFactory factory;

// Directory navigator
DirectoryNavigator nav(&fileSystem);

// Custom command - YOU own this, YOU must delete it
RebootCommand rebootCmd;

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
    fileSystem.begin();
#endif

    // Register built-in commands via factory
    // Factory OWNS these commands and will delete them when it's destroyed
    factory.registerFilesystemCommands(term, nav, CMD_LS | CMD_CD | CMD_CAT);
    factory.registerHelpCommand(term);

    // Register custom command
    // YOU own this command and must ensure it lives as long as Terminal uses it
    term.registerCommand("reboot", &rebootCmd);

    Serial.println("Ownership model:");
    Serial.println("  - Built-in commands (ls, cd, cat, help): Factory owns");
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

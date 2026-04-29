/*
 * BasicTerminal Example
 *
 * This example demonstrates basic usage of the EmbeddedTerminal library
 * on ESP32 or Arduino platforms. It creates a terminal with several
 * built-in commands accessible via Serial using the new simplified
 * command registration API.
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
 * 1. Upload this sketch to your ESP32/Arduino board
 * 2. Open Serial Monitor (115200 baud)
 * 3. Type commands and press Enter
 *
 * Example session:
 *   > help
 *   > ls
 *   > mkdir test
 *   > cd test
 *   > df
 */

#include <Arduino.h>
#include <Terminal.h>
#include <DirectoryNavigator.h>
#include <StorageSystem.h>
#include <commands/BuiltinCommands.h>
#include <interfaces/IStorage.h>

// Platform-specific file system
#if defined(ESP32)
#include <SPIFFS.h>
#include <hal/arduino/ArduinoFileSystem.h>
#include <hal/espidf/ESPNetworkInterface.h>
EmbeddedTerminal::ArduinoFileSystem fileSystem(SPIFFS);
EmbeddedTerminal::ESPNetworkInterface networkInterface;
#elif defined(ARDUINO)
#include <hal/arduino/ArduinoFileSystem.h>
#include <SD.h>
EmbeddedTerminal::ArduinoFileSystem fileSystem(SD);
// Note: Network interface not available on basic Arduino
#else
#include <hal/native/NativeFileSystem.h>
EmbeddedTerminal::NativeFileSystem fileSystem;
#endif

using namespace EmbeddedTerminal;

// Create terminal instance (Terminal accepts Stream directly on Arduino)
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

#if defined(ESP32)
class SingleNetworkSystem : public INetworkSystem
{
public:
    explicit SingleNetworkSystem(INetworkInterface &iface) : iface_(&iface) {}
    ETVector<INetworkInterface *> interfaces() const override { return ETVector<INetworkInterface *>{iface_}; }
    INetworkInterface *getInterface(const ETString &name) const override
    {
        auto info = iface_->info();
        return (info.name == name) ? iface_ : nullptr;
    }
    bool addInterface(const ETString &, INetworkInterface *) override { return false; }
    bool removeInterface(const ETString &) override { return false; }
    ETString ping(const ETString &interfaceName, const ETString &target) override
    {
        auto iface = getInterface(interfaceName);
        return iface ? iface->ping(target) : "Interface not found\n";
    }
    ETString ping(const ETString &target) override { return iface_->ping(target); }

private:
    INetworkInterface *iface_;
} networkSystem(networkInterface);

cmd::cat catCommand(nav);
cmd::cd cdCommand(nav);
cmd::download downloadCommand(nav);
cmd::ls lsCommand(nav);
cmd::mkdir mkdirCommand(nav);
cmd::rm rmCommand(nav);
cmd::rmdir rmdirCommand(nav);
cmd::tail tailCommand(nav);
cmd::pwd pwdCommand(nav);
cmd::xxd xxdCommand(nav);
cmd::touch touchCommand(nav);
cmd::echo echoCommand;
cmd::wc wcCommand(nav);
cmd::df dfCommand(storage);
cmd::help helpCommand(term);
cmd::ip ipCommand(networkSystem);
cmd::ping pingCommand(networkSystem);
#else
cmd::cat catCommand(nav);
cmd::cd cdCommand(nav);
cmd::download downloadCommand(nav);
cmd::ls lsCommand(nav);
cmd::mkdir mkdirCommand(nav);
cmd::rm rmCommand(nav);
cmd::rmdir rmdirCommand(nav);
cmd::tail tailCommand(nav);
cmd::pwd pwdCommand(nav);
cmd::xxd xxdCommand(nav);
cmd::touch touchCommand(nav);
cmd::echo echoCommand;
cmd::wc wcCommand(nav);
cmd::df dfCommand(storage);
cmd::help helpCommand(term);
#endif

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
    term.registerCommand("download", &downloadCommand);
    term.registerCommand("ls", &lsCommand);
    term.registerCommand("mkdir", &mkdirCommand);
    term.registerCommand("rm", &rmCommand);
    term.registerCommand("rmdir", &rmdirCommand);
    term.registerCommand("tail", &tailCommand);
    term.registerCommand("pwd", &pwdCommand);
    term.registerCommand("xxd", &xxdCommand);
    term.registerCommand("touch", &touchCommand);
    term.registerCommand("echo", &echoCommand);
    term.registerCommand("wc", &wcCommand);
    term.registerCommand("df", &dfCommand);
    term.registerCommand("help", &helpCommand);
#if defined(ESP32)
    term.registerCommand("ip", &ipCommand);
    term.registerCommand("ping", &pingCommand);
#endif

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

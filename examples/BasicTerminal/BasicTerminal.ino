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
#include <BuiltinCommandFactory.h>
#include <DirectoryNavigator.h>
#include <StorageSystem.h>
#include <interfaces/IStorage.h>

// Platform-specific file system
#if defined(ESP32)
#include <SPIFFS.h>
#include <hal/ArduinoFileSystem.h>
#include <hal/ESPNetworkInterface.h>
ArduinoFileSystem fileSystem(SPIFFS);
ESPNetworkInterface networkInterface;
#elif defined(ARDUINO)
#include <hal/ArduinoFileSystem.h>
#include <SD.h>
ArduinoFileSystem fileSystem(SD);
// Note: Network interface not available on basic Arduino
#else
#include <hal/NativeFileSystem.h>
NativeFileSystem fileSystem;
#endif

using namespace EmbeddedTerminal;

// Create terminal instance (Terminal accepts Stream directly on Arduino)
Terminal term(Serial);

// Create factory instance (owns built-in commands)
EmbeddedTerminal::BuiltinCommandFactory factory;

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

    // Register all built-in commands using BuiltinCommandFactory
    // This is the simplest approach - all commands registered automatically
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

    factory.registerAllCommands(term, nav, networkSystem);
#else
    // On platforms without network support, register commands individually by category
    factory.registerFilesystemCommands(term, nav); // cat, cd, download, ls, mkdir, rm, rmdir, tail
    factory.registerDiskCommands(term, nav);       // df
    factory.registerHelpCommand(term);             // help
#endif

    // Alternative: Selective registration with flags
    // Uncomment to register only specific commands:
    // factory.registerFilesystemCommands(term, nav, CMD_LS | CMD_CD | CMD_CAT);
    // factory.registerDiskCommands(term, nav, CMD_DF);
    // factory.registerHelpCommand(term);

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

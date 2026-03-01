/*
 * DualNetworkTerminal Example
 *
 * This example demonstrates how to register the same command type with
 * different keywords for different network interfaces (e.g., Ethernet and WiFi).
 *
 * Commands available:
 * - help      : List all available commands
 * - eth-ip    : Show Ethernet network interface info
 * - wifi-ip   : Show WiFi network interface info
 * - ls, cd, cat, etc. : File system commands
 *
 * Usage:
 * 1. Upload this sketch to your board with dual network capabilities
 * 2. Open Serial Monitor (115200 baud)
 * 3. Try commands:
 *    > eth-ip
 *    > wifi-ip
 *    > help
 */

#include <Arduino.h>
#include <Terminal.h>
#include <DirectoryNavigator.h>
#include <StorageSystem.h>
#include <commands/ip.h>
#include <BuiltinCommandFactory.h>
#include <interfaces/INetworkInterface.h>
#include <interfaces/IStorage.h>

// Use appropriate file system for your platform
#if defined(ESP32)
#include <SPIFFS.h>
#include <hal/ArduinoFileSystem.h>
ArduinoFileSystem fileSystem(SPIFFS);
#else
#include <hal/ArduinoFileSystem.h>
#include <SD.h>
ArduinoFileSystem fileSystem(SD);
#endif

using namespace EmbeddedTerminal;

// Create terminal instance
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

class StaticNetworkInterface : public INetworkInterface
{
public:
    explicit StaticNetworkInterface(const NetworkInfo &info) : info_(info) {}
    NetworkInfo info() const override { return info_; }
    ETString ping(const ETString &target) override
    {
        if (!info_.isUp)
            return "Interface is down\n";
        return "Ping " + target + " via " + info_.name + " successful\n";
    }

private:
    NetworkInfo info_;
};

class DualNetworkSystem : public INetworkSystem
{
public:
    DualNetworkSystem(INetworkInterface &eth, INetworkInterface &wifi)
        : interfaces_{&eth, &wifi}
    {
    }

    ETVector<INetworkInterface *> interfaces() const override
    {
        return interfaces_;
    }

    INetworkInterface *getInterface(const ETString &name) const override
    {
        for (auto *iface : interfaces_)
        {
            if (iface->info().name == name)
                return iface;
        }
        return nullptr;
    }

    bool addInterface(const ETString &, INetworkInterface *) override { return false; }
    bool removeInterface(const ETString &) override { return false; }

    ETString ping(const ETString &interfaceName, const ETString &target) override
    {
        auto *iface = getInterface(interfaceName);
        return iface ? iface->ping(target) : "Interface not found\n";
    }

    ETString ping(const ETString &target) override
    {
        for (auto *iface : interfaces_)
        {
            if (iface->info().isUp)
                return iface->ping(target);
        }
        return "No active interface\n";
    }

private:
    ETVector<INetworkInterface *> interfaces_;
};

class SingleNetworkSystem : public INetworkSystem
{
public:
    explicit SingleNetworkSystem(INetworkInterface &iface) : iface_(&iface) {}

    ETVector<INetworkInterface *> interfaces() const override { return ETVector<INetworkInterface *>{iface_}; }
    INetworkInterface *getInterface(const ETString &name) const override
    {
        return iface_->info().name == name ? iface_ : nullptr;
    }
    bool addInterface(const ETString &, INetworkInterface *) override { return false; }
    bool removeInterface(const ETString &) override { return false; }
    ETString ping(const ETString &interfaceName, const ETString &target) override
    {
        auto *iface = getInterface(interfaceName);
        return iface ? iface->ping(target) : "Interface not found\n";
    }
    ETString ping(const ETString &target) override { return iface_->ping(target); }

private:
    INetworkInterface *iface_;
};

StorageSystem storage;
ExampleStorageMedia media("default", &fileSystem);
DirectoryNavigator nav(&storage);

// Create separate network interface instances
StaticNetworkInterface ethernetInterface(NetworkInfo("eth0", "192.168.1.100", "00:11:22:33:44:55", "255.255.255.0", "192.168.1.1", true));
StaticNetworkInterface wifiInterface(NetworkInfo("wlan0", "192.168.2.200", "AA:BB:CC:DD:EE:FF", "255.255.255.0", "192.168.2.1", true));
DualNetworkSystem networkSystem(ethernetInterface, wifiInterface);
SingleNetworkSystem ethernetOnlySystem(ethernetInterface);
SingleNetworkSystem wifiOnlySystem(wifiInterface);

// Create separate IP command instances for each network
cmd::ip ethIpCommand(ethernetOnlySystem);
cmd::ip wifiIpCommand(wifiOnlySystem);

void setup()
{
    // Initialize Serial communication
    Serial.begin(115200);
    delay(1000);

    // Welcome message
    Serial.println("\n\n");
    Serial.println("==========================================");
    Serial.println("  Dual Network Terminal Example");
    Serial.println("==========================================");
    Serial.println();

// Initialize file system
#if defined(ESP32)
    if (!SPIFFS.begin(true))
    {
        Serial.println("ERROR: Failed to mount SPIFFS!");
    }
    else
    {
        Serial.println("File system initialized");
    }
#else
    SD.begin();
#endif

    storage.mountMedia(&media, "");

    // Register filesystem and disk commands using factory
    factory.registerFilesystemCommands(term, nav, CMD_LS | CMD_CD | CMD_CAT);
    factory.registerDiskCommands(term, nav);
    factory.registerNetworkCommands(term, networkSystem, CMD_IP);
    factory.registerHelpCommand(term);

    // Register SEPARATE IP commands with DIFFERENT keywords
    // This demonstrates registering the same command type with different instances
    term.registerCommand("eth-ip", &ethIpCommand);
    term.registerCommand("wifi-ip", &wifiIpCommand);

    // Alternative approach: register under generic "ip" keyword with one interface
    // term.registerCommand("ip", &ethIpCommand);

    // Show usage
    Serial.println("Dual network interface configuration:");
    Serial.println("  - Ethernet: eth-ip");
    Serial.println("  - WiFi:     wifi-ip");
    Serial.println();
    Serial.println("File commands: ls, cd, cat, df");
    Serial.println("Type 'help' to see all available commands");
    Serial.println();
    Serial.println("Example usage:");
    Serial.println("  > eth-ip        # Show Ethernet interface info");
    Serial.println("  > wifi-ip       # Show WiFi interface info");
    Serial.println("  > ls            # List files");
    Serial.print("\n> ");
}

void loop()
{
    // Process terminal input
    term.loop();
}

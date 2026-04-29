/*
 * NetworkTerminal Example
 *
 * This example demonstrates how to create a terminal with network support
 * on ESP32. It includes the 'ip' command to display network interface
 * information (WiFi and/or Ethernet).
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
 * 1. Update WiFi credentials below (SSID and password)
 * 2. Upload this sketch to your ESP32 board
 * 3. Open Serial Monitor (115200 baud)
 * 4. Type commands and press Enter
 *
 * Example session:
 *   > help
 *   > ip
 *   > ls
 *   > df
 */

#include <Arduino.h>
#include <Terminal.h>
#include <DirectoryNavigator.h>
#include <StorageSystem.h>
#include <commands/BuiltinCommands.h>
#include <interfaces/IStorage.h>

// ESP32-specific includes
#if defined(ESP32)
#include <WiFi.h>
#include <SPIFFS.h>
#include <hal/arduino/ArduinoFileSystem.h>
#include <hal/espidf/ESPNetworkInterface.h>

// WiFi credentials - UPDATE THESE!
const char *ssid = "SSID";
const char *password = "password";

EmbeddedTerminal::ArduinoFileSystem fileSystem(SPIFFS);
EmbeddedTerminal::ESPNetworkInterface networkInterface;
#else
#error "This example is designed for ESP32 only"
#endif

using namespace EmbeddedTerminal;

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

class SingleNetworkSystem : public INetworkSystem
{
public:
    SingleNetworkSystem(const ETString &name, INetworkInterface &iface) : name_(name), iface_(&iface) {}

    ETVector<INetworkInterface *> interfaces() const override { return ETVector<INetworkInterface *>{iface_}; }

    INetworkInterface *getInterface(const ETString &name) const override
    {
        return (name == name_) ? iface_ : nullptr;
    }

    bool addInterface(const ETString &, INetworkInterface *) override { return false; }
    bool removeInterface(const ETString &) override { return false; }

    ETString ping(const ETString &interfaceName, const ETString &target) override
    {
        auto iface = getInterface(interfaceName);
        return iface ? iface->ping(target) : "Unknown interface\n";
    }

    ETString ping(const ETString &target) override
    {
        return iface_->ping(target);
    }

private:
    ETString name_;
    INetworkInterface *iface_;
};

StorageSystem storage;
ExampleStorageMedia media("default", &fileSystem);
DirectoryNavigator nav(&storage);
Terminal term(Serial);
SingleNetworkSystem networkSystem("wlan0", networkInterface);

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
cmd::ip ipCommand(networkSystem);
cmd::ping pingCommand(networkSystem);
cmd::help helpCommand(term);

void setup()
{
    // Initialize Serial communication
    Serial.begin(115200);
    delay(1000); // Wait for Serial to initialize

    // Clear screen and show welcome message
    Serial.println("\n\n");
    Serial.println("=======================================");
    Serial.println("  Network Terminal Example (ESP32)");
    Serial.println("=======================================");
    Serial.println();

    // Connect to WiFi
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println();
        Serial.println("WiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println();
        Serial.println("WARNING: WiFi connection failed!");
        Serial.println("Network commands may not work properly.");
    }
    Serial.println();

    if (!SPIFFS.begin(true))
    {
        Serial.println("WARNING: SPIFFS mount failed");
    }
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
    term.registerCommand("ip", &ipCommand);
    term.registerCommand("ping", &pingCommand);
    term.registerCommand("help", &helpCommand);

    // Show available commands
    Serial.println("Terminal ready! Type 'help' for available commands.");
    Serial.println("Try 'ip' to see network interface information.");
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

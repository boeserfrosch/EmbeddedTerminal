# EmbeddedTerminal

**EmbeddedTerminal** is a platform-independent C++ library for embedded systems providing a complete terminal/shell implementation with command parsing, file system abstraction, and network interfaces. Works seamlessly across Arduino, ESP-IDF, and native platforms.

## Disclaimer

The Readme is partially generated using AI but was proven to be inaccurate in some places. Please refer to the source code and documentation for the most accurate information.

## Features

- ✅ **Platform Independent**: Works on Arduino, ESP32 (ESP-IDF & Arduino), and native environments
- ✅ **Command System**: Register and execute custom commands with keyword-based parsing
- ✅ **Command Runtime v2 (Preview)**: Stream-oriented command execution path with exit-code support
- ✅ **Storage System Abstraction**: Unified mountable storage model via `IStorageSystem` + `IStorageMedia`
- ✅ **Built-in Commands**: cat, cd, ls, mkdir, rm, rmdir, pwd, xxd, df, tail, help, ip, ping, download
- ✅ **Network System**: Supports single or multiple interfaces through `INetworkSystem`
- ✅ **Cross-Platform String Handling**: Custom `ETString` class works across all platforms
- ✅ **Directory Navigation**: Full path traversal and manipulation
- ✅ **Comprehensive Testing**: Mock implementations and test suites included

## Installation

### PlatformIO (Recommended)

Add to your `platformio.ini`:

```ini
[env]
lib_deps = 
    boeserfrosch/EmbeddedTerminal@^0.2.1
```

### Arduino Library Manager

1. Open Arduino IDE
2. Go to Sketch → Include Library → Manage Libraries
3. Search for "EmbeddedTerminal"
4. Click Install

### Manual Installation

1. [Download or clone this repository](https://github.com/boeserfrosch/EmbeddedTerminal)
2. Copy the folder to your libraries directory:
   - Arduino: `Documents/Arduino/libraries/`
   - PlatformIO: Include in `lib_deps` or place in `lib/`

## Quick Start

### Basic Terminal Setup (Recommended)

> **Full example:** [examples/BasicTerminal/BasicTerminal.ino](examples/BasicTerminal/BasicTerminal.ino)

```cpp
#include <Terminal.h>
#include <BuiltinCommandFactory.h>
#include <StorageSystem.h>
#include <interfaces/IStorage.h>
#include <hal/arduino/ArduinoFileSystem.h>

class ExampleStorageMedia : public IStorageMedia {
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

ArduinoFileSystem fs(SD);
StorageSystem storage;
ExampleStorageMedia media("default", &fs);
DirectoryNavigator nav(&storage);
Terminal term(Serial);
BuiltinCommandFactory factory;

void setup() {
    Serial.begin(115200);
    SD.begin();
    storage.mountMedia(&media, "");
    
    // Register filesystem + disk + help commands
    // (registerAllCommands requires a valid INetworkSystem)
    factory.registerFilesystemCommands(term, nav);
    factory.registerDiskCommands(term, nav);
    factory.registerHelpCommand(term);
    
    Serial.println("Terminal ready! Type 'help' for available commands.");
    Serial.print("> ");
}

void loop() {
    term.loop();
}
```

### Selective Command Registration

Register only specific command categories using flags:

> **Full example:** [examples/SelectiveRegistration/SelectiveRegistration.ino](examples/SelectiveRegistration/SelectiveRegistration.ino)

```cpp
#include <BuiltinCommandFlags.h>

BuiltinCommandFactory factory;

void setup() {
    Serial.begin(115200);
    
    // Register only filesystem navigation commands
    factory.registerFilesystemCommands(term, nav, CMD_LS | CMD_CD | CMD_CAT);
    
    // Register disk usage command
    factory.registerDiskCommands(term, nav);
    
    // Register network commands (requires INetworkSystem)
    // factory.registerNetworkCommands(term, networkSystem);
    
    // Register help command
    factory.registerHelpCommand(term);
    
    Serial.println("Terminal ready! Type 'help' for available commands.");
    Serial.print("> ");
}
```

### With Network Interface (ESP32)

> **Full example:** [examples/NetworkTerminal/NetworkTerminal.ino](examples/NetworkTerminal/NetworkTerminal.ino)

```cpp
#include <Terminal.h>
#include <BuiltinCommandFactory.h>
#include <StorageSystem.h>
#include <interfaces/IStorage.h>
#include <interfaces/INetworkInterface.h>
#include <hal/espidf/ESPNetworkInterface.h>

class SingleNetworkSystem : public INetworkSystem {
public:
    SingleNetworkSystem(const ETString &name, INetworkInterface &iface) : name_(name), iface_(&iface) {}
    ETVector<INetworkInterface *> interfaces() const override { return ETVector<INetworkInterface *>{iface_}; }
    INetworkInterface *getInterface(const ETString &name) const override { return name == name_ ? iface_ : nullptr; }
    bool addInterface(const ETString &, INetworkInterface *) override { return false; }
    bool removeInterface(const ETString &) override { return false; }
    ETString ping(const ETString &interfaceName, const ETString &target) override {
        auto *iface = getInterface(interfaceName);
        return iface ? iface->ping(target) : "Unknown interface\n";
    }
    ETString ping(const ETString &target) override { return iface_->ping(target); }
private:
    ETString name_;
    INetworkInterface *iface_;
};

ArduinoFileSystem fs(SPIFFS);
StorageSystem storage;
DirectoryNavigator nav(&storage);
ESPNetworkInterface netInterface;
SingleNetworkSystem netSystem("wlan0", netInterface);
Terminal term(Serial);
BuiltinCommandFactory factory;

void setup() {
    Serial.begin(115200);
    WiFi.begin("SSID", "password");
    SPIFFS.begin(true);
    // mount storage media before using DirectoryNavigator-backed commands
    // (see examples for a complete IStorageMedia implementation)
    
    // Register all commands including network
    factory.registerAllCommands(term, nav, netSystem);
    
    Serial.println("Terminal ready! Type 'help' for available commands.");
    Serial.print("> ");
}

void loop() {
    term.loop();
}
```

### Custom Commands

> **Full example:** [examples/SimpleCustomCommand/SimpleCustomCommand.ino](examples/SimpleCustomCommand/SimpleCustomCommand.ino)

```cpp
#include <Terminal.h>
#include <interfaces/ICommand.h>
#include <DirectoryNavigator.h>
#include <StorageSystem.h>
#include <interfaces/IStorage.h>

#if defined(ESP32)
#include <SPIFFS.h>
#include <hal/arduino/ArduinoFileSystem.h>
ArduinoFileSystem fs(SPIFFS);
#else
#include <SD.h>
#include <hal/arduino/ArduinoFileSystem.h>
ArduinoFileSystem fs(SD);
#endif

class ExampleStorageMedia : public IStorageMedia {
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

class MyCommand : public ICommand {
public:
    ETString trigger(const ETString &keyword, const ETString &additional) override {
        return "Hello from custom command!";
    }
    
    ETString usage(const ETString &keyword) override {
        return "mycmd - Custom command example";
    }
};

StorageSystem storage;
ExampleStorageMedia media("default", &fs);
DirectoryNavigator nav(&storage);
MyCommand myCmd;
Terminal term(Serial);

void setup() {
    Serial.begin(115200);
    storage.mountMedia(&media, "");
    
    // Register custom command
    term.registerCommand("mycmd", &myCmd);
    
    Serial.println("Terminal ready! Type 'help' for available commands.");
    Serial.print("> ");
}

void loop() {
    term.loop();
}
```

> **See also:** [examples/CustomCommand/CustomCommand.ino](examples/CustomCommand/CustomCommand.ino) for more advanced custom command examples (echo, uptime, LED control)

### Command Runtime v2 (Preview)

EmbeddedTerminal now includes a new `execute()` command path designed for streaming and scripting use-cases:

- `ICommand::execute(CommandInvocation&)` receives `stdin`, `stdout`, `stderr`, and command context
- `CommandResult` carries command `exitCode` and execution state
- Existing `trigger(keyword, additional)` commands continue to work via the default adapter implementation

This allows incremental migration of commands from string-return APIs to stream-first execution without breaking existing command implementations.

### Auto Completion

EmbeddedTerminal supports TAB-based auto completion for commands and file paths. When the user presses TAB while typing, the terminal:

1. Looks up the command being typed
2. Asks that command for completion suggestions
3. Auto-fills the longest common prefix
4. Displays remaining options if multiple matches exist

#### Using Auto Completion

No setup required! Auto completion works automatically with built-in commands that support it:

- **`cd <TAB>`**: Suggests available directories
- **`cat <TAB>`**: Suggests available files and directories
- **`ls <TAB>`**: Suggests available directories
- **Any path argument**: Auto-complete works mid-word on any path

#### Example Usage

```txt
> cd /t[TAB]
> cd /tmp/
  data/
  logs/
  cache/
```

If only one match, auto-completes immediately:

```txt
> cat /data/m[TAB]
> cat /data/message.txt
```

#### Adding Auto Completion to Custom Commands

To add auto completion to your custom command, override `getSuggestions(...)` from `ICommand`:

```cpp
#include <Terminal.h>
#include <interfaces/ICommand.h>
#include <DefaultAutoCompleters.h>

class MyAutocompleteCommand : public ICommand {
public:
    MyAutocompleteCommand(DirectoryNavigator &nav) : nav_(nav) {}
    
    ETString trigger(const ETString &keyword, const ETString &additional) override {
        return "Command: " + additional;
    }
    
    ETString usage(const ETString &keyword) override {
        return "myautocmd [argument] - Command with auto completion";
    }
    
    // Provide auto completion suggestions
    ETVector<ETString> getSuggestions(const ETString &partial) override {
        ETVector<ETString> suggestions;
        
        // Return suggestions that match the partial input
        // Example: suggest files in current directory
        ETVector<ETString> files = nav_.ls("/");
        for (const auto &file : files) {
            if (file.startsWith(partial)) {
                suggestions.push_back(file);
            }
        }
        
        return suggestions;
    }

private:
    DirectoryNavigator &nav_;
};

// Register like any other command
MyAutocompleteCommand myCmd(nav);
term.registerCommand("myautocmd", &myCmd);
```

**Built-in Auto Completers** (available in [src/DefaultAutoCompleters.h](src/DefaultAutoCompleters.h)):

- **`CommandCompleter`**: Suggests available command names from the terminal's command map
- **`FilePathCompleter`**: Suggests files and directories that match the partial path
- **`DirectoryCompleter`**: Suggests only directories (useful for `cd`, `ls`, etc.)

You can use these in your own commands:

```cpp
class MyCommand : public ICommand {
    DirectoryNavigator &nav_;
    
    ETVector<ETString> getSuggestions(const ETString &partial) override {
        // Use DirectoryCompleter to suggest directories
        DirectoryCompleter completer(nav_);
        return completer.getSuggestions(partial);
    }
};
```

> `ICommand` already inherits auto-completion support via `IAutoCompleter`.

## Supported Platforms

| Platform | Framework | Status | Notes |
| ---------- | ----------- | -------- | ------- |
| ESP32-S3 | ESP-IDF | ✅ Tested | Full support |
| ESP32-S3 | Arduino | ✅ Tested | Full support |
| ESP32 | ESP-IDF | ✅ Compatible | Should work |
| ESP32 | Arduino | ✅ Compatible | Should work |
| Arduino | Arduino | ✅ Compatible | Requires ArxContainer |
| Native | Desktop C++ | ✅ Tested | For testing/development |

## Built-in Commands

| Command | Description | Example |
| --------- | ------------- | --------- |
| `help` | List available commands | `help` |
| `cat` | Display file contents | `cat file.txt` |
| `cd` | Change directory | `cd /folder` |
| `ls` | List directory contents | `ls`, `ls -l`, `ls -d` |
| `mkdir` | Create directory | `mkdir newfolder` |
| `rm` | Remove file | `rm file.txt` |
| `rmdir` | Remove directory | `rmdir folder` |
| `df` | Show disk usage | `df` |
| `tail` | Show end of file | `tail file.txt` |
| `pwd` | Show current directory | `pwd` |
| `xxd` | Hex dump file contents | `xxd firmware.bin` |
| `ip` | Show network interfaces | `ip`, `ip eth0` |
| `ping` | Ping host through network system | `ping 8.8.8.8` |
| `download` | Stream file content over terminal protocol | `download /log.txt` |

## API Reference

### BuiltinCommandFactory

Factory class for creating and managing built-in commands.

```cpp
class BuiltinCommandFactory {
public:
    // Register all built-in commands
    void registerAllCommands(Terminal &term, DirectoryNavigator &nav, INetworkSystem &net);
    
    // Register command categories
    void registerFilesystemCommands(Terminal &term, DirectoryNavigator &nav, uint32_t flags = CMD_FILESYSTEM_ALL);
    void registerDiskCommands(Terminal &term, DirectoryNavigator &nav, uint32_t flags = CMD_DISK_ALL);
    void registerNetworkCommands(Terminal &term, INetworkSystem &net, uint32_t flags = CMD_NETWORK_ALL);
    void registerHelpCommand(Terminal &term);

    // Deregister command categories
    void deregisterFilesystemCommands(Terminal &term, uint32_t flags = CMD_FILESYSTEM_ALL);
    void deregisterDiskCommands(Terminal &term, uint32_t flags = CMD_DISK_ALL);
    void deregisterNetworkCommands(Terminal &term, uint32_t flags = CMD_NETWORK_ALL);
    void deregisterHelpCommand(Terminal &term);
    
    // Deregister all commands
    void deregisterAllCommands(Terminal &term);
    
    // Destructor automatically cleans up all owned commands
    ~BuiltinCommandFactory();
};
```

**Available Command Flags:**

- `CMD_CAT` - Display file contents
- `CMD_CD` - Change directory
- `CMD_DOWNLOAD` - Download file via terminal
- `CMD_LS` - List directory contents
- `CMD_MKDIR` - Create directory
- `CMD_RM` - Remove file
- `CMD_RMDIR` - Remove directory
- `CMD_TAIL` - Display end of file
- `CMD_PWD` - Print current working directory
- `CMD_XXD` - Hex dump file content
- `CMD_DF` - Show disk usage
- `CMD_IP` - Show network interfaces
- `CMD_PING` - Ping host using network system
- `CMD_HELP` - Display help

### Terminal

Main terminal execution engine for processing commands.

```cpp
class Terminal {
public:
    // Constructors
    Terminal(ITerminalStream &input);
    Terminal(Stream &stream);  // Arduino/ESP32 platforms
    
    // Main execution loop
    void loop();
    
    // Command registration (for custom commands)
    void registerCommand(const ETString &keyword, ICommand *command);
    void deregisterCommand(const ETString &keyword);
    
    // Get registered commands
    const ETMap<ETString, ICommand *>& getCommands() const;

    // Last command exit code (127 for unknown command)
    int getLastExitCode() const;
};
```

**Usage Notes:**

- Terminal does NOT own built-in commands - use BuiltinCommandFactory for those
- Custom commands registered via `registerCommand()` must be owned by caller
- Call `loop()` in your main loop to process terminal input
- On Arduino/ESP32, you can pass `Serial` or any `Stream` directly

> **See also:** [examples/OwnershipExample/OwnershipExample.ino](examples/OwnershipExample/OwnershipExample.ino) for a clear demonstration of the ownership model

### File System Integration

```cpp
#include <hal/native/NativeFileSystem.h>
#include <StorageSystem.h>
#include <interfaces/IStorage.h>
#include <DirectoryNavigator.h>

NativeFileSystem fs;
StorageSystem storage;

// Provide a media wrapper around your filesystem implementation
class NativeStorageMedia : public IStorageMedia {
public:
    explicit NativeStorageMedia(IFileSystem *fs) : fs_(fs) {}
    const char *name() const override { return "native"; }
    IFileSystem *fileSystem() override { return fs_; }
    bool isAvailable() const override { return true; }
    unsigned long long totalBytes() const override { return 0; }
    unsigned long long usedBytes() const override { return 0; }
    unsigned long long capacity() const override { return 0; }
    unsigned long long freeBytes() const override { return 0; }
private:
    IFileSystem *fs_;
};

NativeStorageMedia media(&fs);
storage.mountMedia(&media, "");
DirectoryNavigator nav(&storage);

// Access storage system from navigator
IStorageSystem* storagePtr = nav.getStorageSystem();
```

### Default Storage Media (New)

The library now provides ready-to-use storage media wrappers, split by platform:

- [src/hal/common/StorageMediaAdapter.h](src/hal/common/StorageMediaAdapter.h): generic adapter for custom file systems
- [src/hal/arduino/ArduinoSDMMCStorageMedia.h](src/hal/arduino/ArduinoSDMMCStorageMedia.h): Arduino ESP32 default using `SD_MMC`
- [src/hal/espidf/ESPIDFSDMMCStorageMedia.h](src/hal/espidf/ESPIDFSDMMCStorageMedia.h): ESP-IDF default using an SD_MMC mount point (default `/sdcard`)
- [src/hal/native/NativeSuggestedStorageMedia.h](src/hal/native/NativeSuggestedStorageMedia.h): native suggested default backed by `NativeFileSystem`

You can include all of them through [src/hal/common/DefaultStorageMedia.h](src/hal/common/DefaultStorageMedia.h).

```cpp
#include <StorageSystem.h>
#include <hal/common/DefaultStorageMedia.h>

StorageSystem storage;

#if defined(ARDUINO) && defined(ESP32)
ArduinoSDMMCStorageMedia media;
media.begin();
storage.mountMedia(&media, "");
#elif defined(ESP_PLATFORM) || defined(ESP_32)
ESPIDFSDMMCStorageMedia media("sdmmc", "/sdcard");
storage.mountMedia(&media, "");
#else
NativeSuggestedStorageMedia media("native", ".");
storage.mountMedia(&media, "");
#endif

DirectoryNavigator nav(&storage);
```

> **See also:** [examples/DefaultStorageMedia/DefaultStorageMedia.ino](examples/DefaultStorageMedia/DefaultStorageMedia.ino)

### Network Interface

```cpp
#include <hal/espidf/ESPNetworkInterface.h>  // ESP32 WiFi/Ethernet

ESPNetworkInterface netInterface;
// Wrap one or more interfaces into an INetworkSystem implementation.

// Use with network commands
factory.registerNetworkCommands(term, networkSystem);
```

### ICommand Interface

```cpp
class ICommand {
public:
    virtual ETString usage(const ETString &keyword) = 0;
    virtual CommandResult execute(CommandInvocation &invocation);

protected:
    virtual ETString trigger(const ETString &keyword, const ETString &additional) = 0;
};
```

`execute()` is the preferred runtime-v2 path. Existing commands can still implement `trigger()` and rely on the default adapter.

### ETString Class

Cross-platform string class with automatic platform adaptation:

```cpp
class ETString {
public:
    ETString();
    ETString(const char *s);
    size_t length() const;
    bool empty() const;
    ETString substr(size_t pos, size_t len = npos) const;
    size_t find(const ETString &str, size_t pos = 0) const;
    ETString trim() const;
    ETString cleanupString() const;
    const char *c_str() const;
    // Operators: +, +=, ==, !=, <, >, <=, >=, []
};
```

### File System Abstraction

```cpp
class IFileSystem {
public:
    virtual ETFile open(const Path &path, const char *mode = FILE_MODE_READ, const bool create = false) = 0;
    virtual bool exists(const Path &path) = 0;
    virtual bool isDirectory(const Path &path) = 0;
    virtual bool isEmpty(const Path &path) = 0;
    virtual bool remove(const Path &path) = 0;
    virtual bool mkdir(const Path &path) = 0;
    virtual bool rmdir(const Path &path) = 0;
    virtual ETVector<Path> list(const Path &path, const ETString &prefix = "") const = 0;
};
```

## Project Structure

```text
EmbeddedTerminal/
├── src/
│   ├── Terminal.h/cpp            # Main terminal engine
│   ├── BuiltinCommandFactory.h/cpp
│   ├── BuiltinCommandFlags.h
│   ├── StorageSystem.h
│   ├── DirectoryNavigator.h      # Path-aware navigation on IStorageSystem
│   ├── ETTypes.h/cpp             # Cross-platform types (ETString, ETVector, ETMap)
│   ├── OptionParser.h/cpp
│   ├── DefaultAutoCompleters.h
│   ├── commands/                 # Built-in command implementations
│   │   ├── cat.h/cpp
│   │   ├── download.h/cpp
│   │   ├── ip.h/cpp
│   │   ├── ping.h/cpp
│   │   ├── pwd.h/cpp
│   │   ├── xxd.h/cpp
│   │   └── ...
│   ├── interfaces/               # Public interfaces and runtime abstractions
│   │   ├── ICommand.h
│   │   ├── ICommandRuntime.h
│   │   ├── ITerminalStream.h
│   │   ├── IStorage.h
│   │   ├── IFileSystem.h
│   │   ├── IDirectoryNavigator.h
│   │   └── INetworkInterface.h
│   └── hal/
│       ├── arduino/              # Arduino streams/filesystem/media wrappers
│       ├── espidf/               # ESP-IDF filesystem/network/media wrappers
│       ├── native/               # Native test/development adapters
│       └── common/               # Shared storage media adapters
├── test/                        # Unit tests
├── unity-app/                   # ESP-IDF unity test app
├── examples/                    # Example sketches
└── library.json                 # PlatformIO metadata
```

## Testing

The library includes comprehensive unit tests using PlatformIO's testing framework:

```bash
# Test on native platform
pio test -e native

# Test on ESP32
pio test -e esp32s3_espressif

# Test on Arduino framework
pio test -e esp32s3_arduino
```

Mock implementations are provided for testing custom commands without hardware.

## Advanced Usage

### Custom File System Implementation

```cpp
class MyFileSystem : public IFileSystem {
public:
    bool exists(const Path &path) override {
        // Your implementation
    }
    // Implement all pure virtual methods
};
```

### Network Interface Integration

> **Full example:** [examples/DualNetworkTerminal/DualNetworkTerminal.ino](examples/DualNetworkTerminal/DualNetworkTerminal.ino)

```cpp
#include <interfaces/INetworkInterface.h>

class MyNetwork : public INetworkInterface {
public:
    NetworkInfo info() const override {
        return NetworkInfo("eth0", "192.168.1.100", "AA:BB:CC:DD:EE:FF", "255.255.255.0", "192.168.1.1", true);
    }

    ETString ping(const ETString &target) override {
        return "Ping " + target + ": reachable";
    }
};
```

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Add tests for new features
4. Ensure all tests pass
5. Submit a pull request

## Known Limitations

- Arduino platforms require ArxContainer for STL-like containers
- Some platforms may have memory constraints with large command sets
- File system operations depend on platform-specific implementations

## What's Next

- Add more built-in commands (grep, find, chmod, etc.)
- Implement command history and auto-completion
- Add scripting support for command sequences
- Improve documentation with more examples
- Add GUI terminal emulation support

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

## Author

Guido Lehne

## Support

- Report issues: [GitHub Issues](https://github.com/boeserfrosch/EmbeddedTerminal/issues)
- Documentation: [GitHub Wiki](https://github.com/boeserfrosch/EmbeddedTerminal/wiki)
- Examples: [examples/](examples/)

---

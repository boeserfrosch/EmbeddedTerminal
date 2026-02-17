# EmbeddedTerminal

**EmbeddedTerminal** is a platform-independent C++ library for embedded systems providing a complete terminal/shell implementation with command parsing, file system abstraction, and network interfaces. Works seamlessly across Arduino, ESP-IDF, and native platforms.

## Disclaimer

The Readme is partially generated using AI but was proven to be inaccurate in some places. Please refer to the source code and documentation for the most accurate information.

## Features

- ✅ **Platform Independent**: Works on Arduino, ESP32 (ESP-IDF & Arduino), and native environments
- ✅ **Command System**: Register and execute custom commands with keyword-based parsing
- ✅ **File System Abstraction**: Unified interface for different file systems (SPIFFS, SD, LittleFS, Native)
- ✅ **Built-in Commands**: cat, cd, ls, mkdir, rm, rmdir, df, tail, help, ip, download
- ✅ **Network Interface**: Abstract network interface for displaying connection information
- ✅ **Cross-Platform String Handling**: Custom `ETString` class works across all platforms
- ✅ **Directory Navigation**: Full path traversal and manipulation
- ✅ **Comprehensive Testing**: Mock implementations and test suites included

## Installation

### PlatformIO (Recommended)

Add to your `platformio.ini`:

```ini
[env]
lib_deps = 
    boeserfrosch/EmbeddedTerminal@^0.1.0
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

### Basic Terminal Setup

```cpp
#include <Terminal.h>
#include <commands/help.h>
#include <commands/ls.h>

using namespace EmbeddedTerminal;

// Create terminal instance (accepts Serial directly on Arduino platforms)
Terminal term(Serial);

void setup() {
    Serial.begin(115200);
    
    // Register built-in commands
    static cmd::help helpCmd(term);
    static cmd::ls lsCmd(fileSystem);
    
    term.registerCommand("help", &helpCmd);
    term.registerCommand("ls", &lsCmd);
    
    Serial.println("Terminal ready! Type 'help' for available commands.");
}

void loop() {
    term.loop();  // Process terminal input
}
```

### Custom Command Example

```cpp
#include <interfaces/ICommand.h>

class MyCommand : public ICommand {
public:
    ETString trigger(ETString &keyword, ETString &additional) override {
        return "Hello from " + keyword + "! Args: " + additional;
    }
    
    ETString usage(ETString &keyword) override {
        return "Usage: " + keyword + " [args]";
    }
};

// In setup():
static MyCommand myCmd;
term.registerCommand("mycmd", &myCmd);
```

### File System Integration

```cpp
#include <hal/NativeFileSystem.h>  // Or ArduinoFileSystem, SDMMCFileSystem
#include <DirectoryNavigator.h>

NativeFileSystem fs;
DirectoryNavigator nav(&fs);

// Use with commands
static cmd::cd cdCmd(nav);
static cmd::ls lsCmd(nav);
term.registerCommand("cd", &cdCmd);
term.registerCommand("ls", &lsCmd);
```

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
| `ip` | Show network interfaces | `ip`, `ip eth0` |
| `download` | Download file via network | `download url` |

## API Reference

### Terminal Class

```cpp
namespace EmbeddedTerminal {
    class Terminal {
    public:
        // Simple constructor for Arduino/ESP32 (accepts Stream directly)
        Terminal(Stream &stream);  // Arduino/ESP32 only
        
        // Advanced constructor for custom stream implementations
        Terminal(ITerminalStream &input);
        
        void loop();
        void registerCommand(ETString keyword, ICommand *observer);
        void call(ETString keyword, ETString additional);
        const ETMap<ETString, ICommand *> getCommands();
    };
}
```

**Usage Notes:**

- On Arduino/ESP32 platforms, you can pass `Serial` or any `Stream` object directly
- For custom stream implementations, create a class implementing `ITerminalStream` interface
- The Stream is managed internally; no manual cleanup required

### ICommand Interface

```cpp
class ICommand {
public:
    virtual ETString trigger(ETString &keyword, ETString &additional) = 0;
    virtual ETString usage(ETString &keyword) = 0;
};
```

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
    virtual bool exists(const char *path) = 0;
    virtual bool mkdir(const char *path) = 0;
    virtual bool rmdir(const char *path) = 0;
    virtual bool remove(const char *path) = 0;
    virtual ETVector<ETFile> list(const char *path) const = 0;
    virtual ETFile open(const char *path, const char *mode = "r", bool create = false) = 0;
    virtual bool isDirectory(const char *path) = 0;
    virtual bool isEmpty(const char *path) = 0;
    virtual unsigned long long capacity() const = 0;
    virtual unsigned long long totalBytes() const = 0;
    virtual unsigned long long usedBytes() const = 0;
};
```

## Project Structure

```
EmbeddedTerminal/
├── src/
│   ├── Terminal.h/cpp           # Main terminal engine
│   ├── ETTypes.h/cpp            # Cross-platform types
│   ├── DirectoryNavigator.h    # Directory navigation
│   ├── ETFile.h                 # File wrapper
│   ├── commands/                # Built-in commands
│   │   ├── cat.h/cpp
│   │   ├── cd.h/cpp
│   │   ├── ls.h/cpp
│   │   ├── help.h/cpp
│   │   └── ...
│   ├── interfaces/              # Abstract interfaces
│   │   ├── ICommand.h
│   │   ├── IFile.h
│   │   ├── IFileSystem.h
│   │   ├── ITerminalStream.h
│   │   └── INetworkInterface.h
│   └── hal/                     # Hardware abstraction
│       ├── NativeFileSystem.h
│       ├── ArduinoFileSystem.h
│       └── SDMMCFileSystem.h
├── test/                        # Unit tests
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
    bool exists(const char *path) override {
        // Your implementation
    }
    // Implement all pure virtual methods
};
```

### Network Interface Integration

```cpp
#include <interfaces/INetworkInterface.h>

class MyNetwork : public INetworkInterface {
public:
    ETVector<NetworkInfo> getAll() override {
        ETVector<NetworkInfo> interfaces;
        NetworkInfo eth0;
        eth0.name = "eth0";
        eth0.ip = "192.168.1.100";
        eth0.mac = "AA:BB:CC:DD:EE:FF";
        interfaces.push_back(eth0);
        return interfaces;
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

**Guido Lehne**

## Support

- Report issues: [GitHub Issues](https://github.com/boeserfrosch/EmbeddedTerminal/issues)
- Documentation: [GitHub Wiki](https://github.com/boeserfrosch/EmbeddedTerminal/wiki)
- Examples: [examples/](examples/)

---

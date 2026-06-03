# EmbeddedTerminal

**EmbeddedTerminal** is a platform-independent C++ library for embedded systems providing a complete terminal/shell implementation with command parsing, file system abstraction, and network interfaces. Works seamlessly across Arduino, ESP-IDF, and native platforms.

## Disclaimer

The Readme is mostly generated using AI. It was reviewed for accuracy. If you find any mistakes or inconsistencies, please open an issue or submit a PR with corrections. Thanks!
For the most accurate and up-to-date information, please refer to the source code and documentation in this repository.

## Features

- ✅ **Platform Independent**: Works on Arduino, ESP32 (ESP-IDF & Arduino), and native environments
- ✅ **Command System**: Register and execute custom commands with keyword-based parsing
- ✅ **Command Runtime v2 (Preview)**: Stream-oriented command execution path with exit-code support
- ✅ **Storage System Abstraction**: Unified mountable storage model via `IStorageSystem` + `IStorageMedia`
- ✅ **Command Registration**: create the command objects you need and register them explicitly
- ✅ **GPIO Control**: Secure, policy-driven GPIO access with compile-time password protection
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
#include <commands/BuiltinCommands.h>
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
cmd::cat catCommand(nav);
cmd::cd cdCommand(nav);
cmd::ls lsCommand(nav);
cmd::df dfCommand(storage);
cmd::help helpCommand(term);

void setup() {
    Serial.begin(115200);
    SD.begin();
    storage.mountMedia(&media, "");
    
    term.registerCommand("cat", &catCommand);
    term.registerCommand("cd", &cdCommand);
    term.registerCommand("ls", &lsCommand);
    term.registerCommand("df", &dfCommand);
    term.registerCommand("help", &helpCommand);
    
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
#include <commands/BuiltinCommands.h>

cmd::cat catCommand(nav);
cmd::cd cdCommand(nav);
cmd::ls lsCommand(nav);
cmd::df dfCommand(storage);
cmd::help helpCommand(term);

void setup() {
    Serial.begin(115200);
    
    term.registerCommand("cat", &catCommand);
    term.registerCommand("cd", &cdCommand);
    term.registerCommand("ls", &lsCommand);
    term.registerCommand("df", &dfCommand);
    term.registerCommand("help", &helpCommand);
    
    Serial.println("Terminal ready! Type 'help' for available commands.");
    Serial.print("> ");
}
```

### With Network Interface (ESP32)

> **Full example:** [examples/NetworkTerminal/NetworkTerminal.ino](examples/NetworkTerminal/NetworkTerminal.ino)

```cpp
#include <Terminal.h>
#include <commands/BuiltinCommands.h>
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
cmd::cat catCommand(nav);
cmd::cd cdCommand(nav);
cmd::download downloadCommand(nav);
cmd::ls lsCommand(nav);
cmd::df dfCommand(storage);
cmd::ip ipCommand(netSystem);
cmd::ping pingCommand(netSystem);
cmd::help helpCommand(term);

void setup() {
    Serial.begin(115200);
    WiFi.begin("SSID", "password");
    SPIFFS.begin(true);
    // mount storage media before using DirectoryNavigator-backed commands
    // (see examples for a complete IStorageMedia implementation)
    
    term.registerCommand("cat", &catCommand);
    term.registerCommand("cd", &cdCommand);
    term.registerCommand("download", &downloadCommand);
    term.registerCommand("ls", &lsCommand);
    term.registerCommand("df", &dfCommand);
    term.registerCommand("ip", &ipCommand);
    term.registerCommand("ping", &pingCommand);
    term.registerCommand("help", &helpCommand);
    
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
    ETString usage(const ETString &keyword) const override {
        return keyword + " [name] - Custom command example";
    }

    CommandResult invoke(CommandInvocation &invocation) override {
        ETString name = invocation.arguments.empty() ? "world" : invocation.arguments[0];
        invocation.streams.output.print("Hello from custom command, " + name + "!\n");
        return CommandResult::completed(0);
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

### Command Runtime

EmbeddedTerminal's command API is stream-based:

- `ICommand::invoke(CommandInvocation&)` receives `input`, `output`, `error`, and command context
- `CommandResult` carries command `exitCode` and execution state
- `ICommand::resume(CommandInvocation&)` continues commands that returned `Running` or `WaitingForInput`

This allows for:

- Long-running commands that can yield and resume
- Interactive commands that can wait for user input
- Commands that can return specific exit codes for scripting and control flow

### Auto Completion

EmbeddedTerminal supports TAB-based auto completion for commands and file paths and any other input that supports it. When the user presses TAB while typing, the terminal:

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
    
    ETString usage(const ETString &keyword) const override {
        return keyword + " [argument] - Command with auto completion";
    }

    CommandResult invoke(CommandInvocation &invocation) override {
        ETString argument = invocation.arguments.empty() ? "" : invocation.arguments[0];
        invocation.streams.output.print("Command: " + argument + "\n");
        return CommandResult::completed(0);
    }
    
    // Provide auto completion suggestions
    ETVector<ETString> getSuggestions(const ETString &partial) const override {
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
    
    ETVector<ETString> getSuggestions(const ETString &partial) const override {
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
| `gpio` | Control GPIO pins with security policies | `gpio read 2`, `gpio mode 5 output` |
| `gpio` | Control GPIO pins with security policies | `gpio read 2`, `gpio mode 5 output` |

### Pipe Support

Commands that support streaming can be piped together using `|`:

```txt
cat file.txt | xxd
cat file.txt | tail
```

Also redirection operators `>` and `<` are supported for commands that support streaming:

```txt
cat file.txt > output.txt
cat < input.txt
echo "Hello" > greeting.txt
echo "Hello" >> greeting.txt  # Append to file 
```

### Script Command Syntax

Use the `script` command to run inline scripts or script files.

```txt
script collect one; collect two
script -f /scripts/demo.et
```

Supported control flow (bash-style):

```txt
# for-loop
for i in 1 2 3; do echo $i; done

# while-loop
while true; do gpio read 10; delay 50; done

# variable assignment
value = 5

# variable usage
echo "The value is $value"

- variables will be expanded in double quotes but not single quotes, similar to bash

# if / elif / else

read11 = gpio read 11
read10 = gpio read 10
if $read11; then echo fault; elif $read10; then echo button; else echo idle; fi

# function definition + call
function blink; do gpio write 20 1; delay 100; gpio write 20 0; delay 100; done
blink; blink
```

Notes:

- Terminators are `done` for loops/functions and `fi` for `if` blocks.
- `delay <ms>` is cooperative and non-blocking for the terminal loop.
- In conditions are commands/pipelines not allowed. The returned value can be saved in variables and so used. The last return code is available in `$?` for use in conditions as well.
- Conditions can contain:
  - Literal values (non-empty strings are true, empty string is false, `0` and `false` is also considered false for convenience)
  - Boolean literals: `true` and `false`
  - Variables (e.g. `$value`, `$read10`, etc.)
  - Boolean operators: `&&` (and), `||` (or), `!` (not)
  - Parentheses for grouping: `if (condition) && (condition); then ...`
  - Comparison operators: `==`, `!=`. For example: `if $read10 == "5"; then ...`. They perform a text comparison
  - Note: Command substitution (e.g. `$(command)`) is not supported in conditions, but you can achieve similar results by saving command output to variables and using those variables in conditions. This is because commands shall be responsive and cooperative with the terminal loop, and allowing command substitution in conditions could lead to blocking behavior if the substituted command takes a long time to execute.

> **See also:**
>
> - [examples/ScriptExample/demo.et](examples/ScriptExample/demo.et) — runnable example script
> - [examples/ScriptExample/README.md](examples/ScriptExample/README.md) — how to run it

### GPIO Commands (Secure Hardware Control)

The `gpio` command family provides secure, policy-driven access to GPIO pins with compile-time configuration for board-specific protection.

**Key Features:**

- **Board Pin Discovery + Policy Gate**: Detect board pins (Using compile-time flags) and apply compile-time policy restrictions
- **Password Protection**: Optional admin authentication using FNV-1a hash (no plaintext storage)
- **Forced Exclusions**: Immutable pin restrictions that cannot be overridden
- **Per-Operation Flags**: Control read/write/mode/exclude/include separately for each pin
- **Cross-Platform**: Works on ESP32 (Arduino & ESP-IDF), with mock support for testing

#### Subcommands

| Subcommand | Description | Example |
| ---------- | ----------- | ------- |
| `gpio list` | Show board pins and current policy status | `gpio list` |
| `gpio policy` | Show current policy configuration | `gpio policy` |
| `gpio read <pin>` | Read digital value from pin | `gpio read 2`, `gpio read GPIO5` |
| `gpio write <pin> <0\|1>` | Write digital value to pin | `gpio write 2 1`, `gpio write GPIO5 0` |
| `gpio mode <pin> <mode>` | Set pin mode | `gpio mode 2 output`, `gpio mode 5 input_pullup` |
| `gpio deny <pin> [ops]` | Deny selected operations for one or more pins | `gpio deny 2`, `gpio deny 5 r,w` |
| `gpio allow <pin> [ops]` | Allow selected operations for one or more pins | `gpio allow 2`, `gpio allow 5 r,w` |
| `gpio excluded` | List all excluded pins | `gpio excluded` |
| `gpio auth <password>` | Authenticate for protected operations | `gpio auth mypassword` |

**Supported Pin Modes:**

- `input` - Input without pull resistors
- `output` - Output mode
- `input_pullup` - Input with pull-up resistor
- `input_pulldown` - Input with pull-down resistor (platform-dependent)

**Operation Flags** (for `gpio deny` / `gpio allow`):

- `r` - Deny read operations
- `w` - Deny write operations  
- `m` - Deny mode changes
- `e` - Deny further exclusions (protected)
- `i` - Deny include (cannot be removed)

#### Configuration (PlatformIO)

Configure GPIO security in your `platformio.ini`:

```ini
[env:esp32]
platform = espressif32
board = esp32-s3-devkitc-1

build_flags = 
    ; Allowed pins (CSV list, optional)
    -DET_GPIO_ALLOWED_PINS=\"GPIO2,4,5,12,13,14,15\"
    
    ; Forced exclusions (immutable, CSV format: "pin:flags")
    -DET_GPIO_FORCED_EXCLUDED_PINS=\"GPIO0:r,w,m,e,i;GPIO45:r,w,m,i;GPIO46:r,w,m,i\"
    
    ; Policy: 0=deny (default), 1=allow
    -DET_GPIO_DEFAULT_ALLOW=0
    
    ; Admin password hash (FNV-1a 32-bit hex)
    -DET_GPIO_ADMIN_PASSWORD_HASH=\"0x1a2b3c4d\"
```

**Generating Password Hash:**

```cpp
// In a test sketch, use the static helper:
#include <hal/common/CompileTimeGpioAuth.h>

void setup() {
    Serial.begin(115200);
    ETString hash = CompileTimeGpioAuth::hashPasswordHex("yourpassword");
    Serial.println("Hash: " + hash);
}
```

#### Usage in Code

**Using DefaultGpioSupport (Recommended):**

```cpp
#include <Terminal.h>
#include <commands/BuiltinCommands.h>
#include <hal/common/DefaultGpioSupport.h>

Terminal term(Serial);

void setup() {
    Serial.begin(115200);
    
    // DefaultGpioSupport reads ET_GPIO_* compile-time macros
    DefaultGpioSupport gpioSupport;
    if (gpioSupport.available() && gpioSupport.gpio() != nullptr) {
        static cmd::gpio gpioCommand(*gpioSupport.gpio(),
                                     gpioSupport.policy(),
                                     gpioSupport.auth());
        term.registerCommand("gpio", &gpioCommand);
    }
    static cmd::help helpCommand(term);
    static cmd::script scriptCommand(term);
    term.registerCommand("help", &helpCommand);
    term.registerCommand("script", &scriptCommand);
    
    Serial.println("GPIO terminal ready!");
}

void loop() {
    term.loop();
}
```

**Custom Implementation:**

```cpp
#include <hal/arduino/ArduinoGpioInterface.h>
#include <hal/common/ConfigurableGpioPolicy.h>
#include <hal/common/CompileTimeGpioAuth.h>

ArduinoGpioInterface gpioHal;

ETVector<ETString> allowed = {"GPIO2", "GPIO4", "GPIO5"};
ETVector<GpioExclusionRule> forced = {
    {"GPIO0", true, true, true, true, true}  // Deny all ops
};
ConfigurableGpioPolicy policy(allowed, forced, true);  // Default deny

CompileTimeGpioAuth auth("0x1a2b3c4d");

factory.registerGpioCommands(term, gpioHal, policy, auth);
```

#### Security Model

1. **Board Pins + Policy**: Board pins are always listed; policy decides which operations are allowed
2. **Forced Exclusions**: Pins with `i` flag cannot be included, even by admin
3. **Protected Exclusions**: Pins with `e` flag require authentication to modify exclusions
4. **Session Auth**: `gpio auth` authenticates for the current session only
5. **No Plaintext**: Passwords are verified against compile-time FNV-1a hash

**Example Security Configuration:**

```ini
; Bootloader pins - completely locked
-DET_GPIO_FORCED_EXCLUDED_PINS=\"GPIO0:r,w,m,e,i;GPIO45:r,w,m,e,i;GPIO46:r,w,m,e,i\"

; Working pins - accessible
-DET_GPIO_ALLOWED_PINS=\"GPIO2,4,5,12,13,14,15\"

; Default policy for unlisted pins: 0=deny, 1=allow
-DET_GPIO_DEFAULT_ALLOW=0

; Optional admin password hash (FNV-1a 32-bit hex)
-DET_GPIO_ADMIN_PASSWORD_HASH=\"0xbf1075ac\"

; If ET_GPIO_ALLOWED_PINS is empty/unset, all detected board pins are policy-addressable

; Critical peripheral pins - admin only
; (These would be in forced exclusions with 'e' flag requiring auth to modify)
```

> **See also:**
>
> - [examples/GpioTerminalArduino/](examples/GpioTerminalArduino/) - Arduino framework example
> - [examples/GpioTerminalESPIDF/](examples/GpioTerminalESPIDF/) - ESP-IDF framework example

## API Reference

### Command Registration

Built-in commands are regular command classes under [src/commands/](src/commands/). Create the command objects you need, keep them alive for as long as they remain registered, and register them with `Terminal::registerCommand(...)`.

```cpp
cmd::help helpCommand(term);
cmd::ls lsCommand(nav);
cmd::df dfCommand(storage);

term.registerCommand("help", &helpCommand);
term.registerCommand("ls", &lsCommand);
term.registerCommand("df", &dfCommand);
```

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

- Terminal does NOT own command objects - keep registered commands alive yourself
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
    virtual ETString usage(const ETString &keyword) const = 0;
    virtual CommandResult invoke(CommandInvocation &invocation) = 0;
    virtual CommandResult resume(CommandInvocation &invocation) { return CommandResult::completed(0); }
};
```

`invoke()` is the primary entry point. Override `resume()` only for commands that can pause and continue later.

### Migration From `trigger()`

Version 0.* used a `trigger(keyword, additional)` pattern. The current API uses `CommandInvocation` instead.

| Old pattern | New pattern |
| --- | --- |
| `ETString trigger(const ETString &keyword, const ETString &additional)` | `CommandResult invoke(CommandInvocation &invocation)` |
| Return a string directly | Print to `invocation.streams.output` or `invocation.streams.error` |
| Parse the trailing text yourself | Read `invocation.arguments` directly |
| No pause/resume support | Return `CommandResult::running(...)` or `waitingForInput(...)` and override `resume(...)` if needed |

Minimal migration example:

```cpp
class MyCommand : public ICommand {
public:
    ETString usage(const ETString &keyword) const override {
        return keyword + " [text] - Example command";
    }

    ETString trigger(const ETString &keyword, const ETString &additional) {
        return "You entered: " + additional;
    }

    CommandResult invoke(CommandInvocation &invocation) override {
        ETString text = join(invocation.arguments, " ");
        ETString result = trigger(invocation.keyword, text);
        invocation.streams.output.print(result + "\n");
        return CommandResult::completed(0);
    }
};
```

If a command needs multiple steps, store state in `invocation.context.variables` and implement `resume(...)`.

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
│   ├── commands/
│   ├── StorageSystem.h
│   ├── DirectoryNavigator.h      # Path-aware navigation on IStorageSystem
│   ├── ETTypes.h/cpp             # Cross-platform types (ETString, ETVector, ETMap)
│   ├── OptionParser.h/cpp
│   ├── DefaultAutoCompleters.h
│   ├── commands/                 # Built-in command implementations
│   │   ├── cat.h/cpp
│   │   ├── download.h/cpp
│   │   ├── gpio.h/cpp
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
│   │   ├── INetworkInterface.h
│   │   ├── IGpioInterface.h
│   │   ├── IGpioPolicy.h
│   │   └── IGpioAuth.h
│   └── hal/
│       ├── arduino/              # Arduino streams/filesystem/media/GPIO wrappers
│       ├── espidf/               # ESP-IDF filesystem/network/media/GPIO wrappers
│       ├── native/               # Native test/development adapters
│       └── common/               # Shared adapters (storage, GPIO policy/auth)
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

- Add more built-in commands (grep, find, etc.)
- Implement command history
- Improve documentation with more examples
- Add redirecting for error streams

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

## Author

Guido Lehne

## Support

- Report issues: [GitHub Issues](https://github.com/boeserfrosch/EmbeddedTerminal/issues)
- Documentation: [GitHub Wiki](https://github.com/boeserfrosch/EmbeddedTerminal/wiki)
- Examples: [examples/](examples/)

---

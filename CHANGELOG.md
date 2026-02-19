# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- **Command runtime v2 scaffolding**
  - Added `ICommandRuntime` abstractions for stream-based command execution (`stdin`, `stdout`, `stderr`)
  - Added `CommandInvocation`, `CommandContext`, and `CommandResult` types
  - Added default `ICommand::execute()` implementation that adapts legacy `trigger()` commands
  - Added channel-aware `ITerminalStream::printTo()` / `printfTo()` extension points

### Changed

- **Terminal execution path**
  - `Terminal::call()` now executes commands through `ICommand::execute()`
  - Added terminal-level last exit code tracking via `Terminal::getLastExitCode()`
  - Unknown commands now set exit code `127` and are emitted via the stderr channel abstraction

## [0.2.0] - 2026-02-17

### Added

- **BuiltinCommandFactory** - Factory pattern for creating and managing built-in commands
  - Instance-based factory with automatic memory management
  - Selective command registration with flags (e.g., `CMD_LS | CMD_CD`)
  - Category-based registration methods (filesystem, disk, network, help)
  - `registerAllCommands()` convenience method
  - Clean ownership model: factory owns built-in commands, user owns custom commands
- **DirectoryNavigator enhancements**
  - Added `getFileSystem()` accessor method to expose underlying IFileSystem
- **ESPNetworkInterface** - ESP32-specific network interface implementation
  - Support for WiFi and Ethernet interfaces
  - Returns NetworkInfo structs with IP, MAC, netmask, gateway
- **Enhanced test coverage** (+45 tests)
  - New test_BuiltinCommandFactory suite (13 tests)
  - New test_DirectoryNavigator suite (22 tests)
  - Enhanced test_Terminal suite (10 additional tests)
  - Tests for command registration, deregistration, navigation, path resolution
- **Example sketches**
  - DualNetworkTerminal - Demonstrates multiple network interface registration
  - OwnershipExample - Shows clear ownership boundaries

### Changed

- **Simplified API** - Removed redundant IFileSystem parameter from factory methods
  - `registerAllCommands(terminal, nav, net)` instead of `(terminal, nav, fs, net)`
  - `registerDiskCommands(terminal, nav)` instead of `(terminal, fs)`
  - DirectoryNavigator provides IFileSystem access via `getFileSystem()`
- **Factory pattern** - Changed from static methods to instance-based
  - Enables RAII and automatic cleanup
  - Supports multiple factory instances
  - Better memory management
- **Terminal ownership** - Terminal no longer owns commands
  - Factory owns built-in commands
  - User manages custom command lifetime
  - Terminal stores only non-owning pointers

### Fixed

- Const-correctness throughout command implementations
- Memory management in factory pattern
- Path resolution edge cases in DirectoryNavigator

## [0.1.0] - 2026-02-17

### Added

- Initial release of EmbeddedTerminal library
- Core terminal engine with command parsing and execution
- Cross-platform ETString class with automatic platform adaptation
- Platform-independent file system abstraction (IFileSystem interface)
- Directory navigation with DirectoryNavigator class
- Network interface abstraction (INetworkInterface)
- Built-in commands:
  - `help` - Display available commands
  - `cat` - Display file contents
  - `cd` - Change directory
  - `ls` - List directory contents with flags (-l, -d)
  - `mkdir` - Create directories
  - `rm` - Remove files
  - `rmdir` - Remove directories
  - `df` - Display disk usage
  - `tail` - Display end of file
  - `ip` - Show network interface information
  - `download` - Download files via network
- Hardware Abstraction Layer (HAL) implementations:
  - NativeFileSystem for desktop/testing
  - ArduinoFileSystem for Arduino platforms
  - SDMMCFileSystem for ESP32 SD card support
- Comprehensive test suite with mock implementations
- Cross-platform support:
  - ESP32 (ESP-IDF framework)
  - ESP32 (Arduino framework)
  - Arduino boards
  - Native/desktop environments
- PlatformIO integration with platform-specific test configurations
- Example sketches:
  - BasicTerminal - Demonstrates basic terminal usage
  - CustomCommand - Shows how to create custom commands

### Documentation

- Comprehensive README with installation, usage, and API reference
- GPL-3.0 license
- Keywords file for Arduino IDE syntax highlighting
- Library metadata (library.json, library.properties)

### Testing

- Unit tests for all major components
- Mock implementations for testing without hardware
- Code coverage support for native builds

[Unreleased]: https://github.com/boeserfrosch/EmbeddedTerminal/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/boeserfrosch/EmbeddedTerminal/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/boeserfrosch/EmbeddedTerminal/releases/tag/v0.1.0

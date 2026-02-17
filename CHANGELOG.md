# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

[Unreleased]: https://github.com/boeserfrosch/EmbeddedTerminal/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/boeserfrosch/EmbeddedTerminal/releases/tag/v0.1.0

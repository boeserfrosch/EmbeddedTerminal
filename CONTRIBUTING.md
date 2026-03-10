# Contributing to EmbeddedTerminal

Thank you for your interest in contributing to EmbeddedTerminal! This document provides guidelines and instructions for contributing.

## Code of Conduct

This project adheres to a code of conduct that all contributors are expected to follow. Please be respectful and constructive in your interactions.

## How Can I Contribute?

### Reporting Bugs

Before creating bug reports, please check the existing issues to avoid duplicates. When creating a bug report, include:

- **Clear title and description**
- **Steps to reproduce** the problem
- **Expected behavior** vs **actual behavior**
- **Platform/environment** details (ESP32, Arduino, etc.)
- **Code samples** or test cases if applicable
- **Error messages** or logs

### Suggesting Enhancements

Enhancement suggestions are welcome! Please provide:

- **Clear use case** - Why is this enhancement needed?
- **Proposed solution** - How should it work?
- **Alternatives considered** - What other approaches did you think of?
- **Impact** - Who benefits from this enhancement?

### Pull Requests

1. **Fork the repository** and create your branch from `main`
2. **Follow the coding style** of the existing codebase
3. **Add tests** for new functionality
4. **Update documentation** as needed
5. **Ensure all tests pass** on all platforms
6. **Write clear commit messages**
7. **Submit your pull request**

## Development Setup

### Requirements

- PlatformIO Core or PlatformIO IDE
- Git
- C++ compiler (for native testing)

### Getting Started

```bash
# Clone the repository
git clone https://github.com/boeserfrosch/EmbeddedTerminal.git
cd EmbeddedTerminal

# Run tests on native platform
pio test -e native

# Build for ESP32
pio run -e esp32s3_espressif

# Run all tests
pio test
```

## Coding Guidelines

### Style

- Use **consistent indenting** (4 spaces for C++)
- Follow **existing naming conventions**:
  - Classes: `PascalCase`
  - Variables/functions: `camelCase`
  - Constants: `UPPER_SNAKE_CASE`
- Add **comments** for complex logic
- Keep functions **focused and short**

### Platform Independence

- Use `ETString` instead of `std::string` or `String`
- Use `ETVector` and `ETMap` instead of direct STL usage
- Test on multiple platforms before submitting
- **Use abstraction layers (interfaces) for platform-specific functionality** - See `INetworkInterface` pattern
- Keep platform-specific code only in HAL implementations, never in command layer

### Abstraction Pattern

The preferred approach is to define abstract interfaces (`INetworkInterface`, `IFileSystem`, etc.) with platform-specific implementations. This keeps your business logic clean and testable.

#### Example: INetworkInterface Pattern

```cpp
// In interfaces/INetworkInterface.h
class INetworkInterface {
public:
    virtual ETString ping(const ETString &target) = 0;  // Pure interface
};

// In hal/espidf/ESPNetworkInterface.h - Arduino implementation
class ESPNetworkInterface : public INetworkInterface {
    ETString ping(const ETString &target) override {
        IPAddress ip;
        if (!ip.fromString(target.c_str())) {
            WiFi.hostByName(target.c_str(), ip);
        }
        return WiFi.ping(ip) > 0 ? "reachable" : "unreachable";
    }
};

// In test/Mocks/MockNetworkInterface.h - Test implementation
class MockNetworkInterface : public INetworkInterface {
    ETString ping(const ETString &target) override {
        return target == "10.255.255.255" ? "unreachable" : "reachable (mock)";
    }
};

// In commands/ping.cpp - Clean, platform-agnostic (runtime v2 path)
CommandResult ping::execute(CommandInvocation &invocation) {
    ETString target = invocation.arguments.trim();
    invocation.stdoutChannel.print(_net.ping(target));
    return CommandResult::completed(0);
}
```

See the actual implementation in `src/commands/ping.cpp` and `src/interfaces/INetworkInterface.h` for a production example.

### Command Runtime (v2)

For new commands, prefer implementing `ICommand::execute(CommandInvocation&)`.

- `execute()` gives you channel-based IO (`stdin`, `stdout`, `stderr`)
- it returns a `CommandResult` (`completed`, `running`, `waitingForInput`)
- it supports stateful commands through `invocation.context.variables`

Legacy `trigger(keyword, additional)` remains supported through the default adapter in `ICommand` for backwards compatibility.

### String Handling Example

```cpp
// Good: Platform-independent
ETString processInput(const ETString &input) {
    ETString result = input.trim();
    return result;
}

// Avoid: Direct platform-specific strings
#if defined(ARDUINO)
String processInput(const String &input) {
    String result = input;
    result.trim();
    return result;
}
#else
std::string processInput(const std::string &input) {
    // Different implementation
}
#endif
```

### Adding Auto Completion to Custom Commands

EmbeddedTerminal supports TAB-based auto completion. To add auto completion to your custom command:

1. **Inherit from `ICommand`** (`ICommand` already includes auto-completion support):

   ```cpp
    class MyCommand : public ICommand {
       // ...
   };
   ```

2. **Implement the `getSuggestions()` method**:

   ```cpp
   ETVector<ETString> getSuggestions(const ETString &partial) override {
       ETVector<ETString> suggestions;
       
       // Get your data (files, options, etc.)
       ETVector<ETString> allItems = getAvailableItems();
       
       // Filter items that start with the partial input
       for (const auto &item : allItems) {
           if (item.startsWith(partial)) {
               suggestions.push_back(item);
           }
       }
       
       return suggestions;
   }
   ```

3. **Use built-in completers if applicable** (from [src/DefaultAutoCompleters.h](src/DefaultAutoCompleters.h)):

   ```cpp
   // For path-based completions
   DirectoryCompleter dirCompleter(navigator);
   return dirCompleter.getSuggestions(partial);
   
   // For file path completions (files and directories)
   FilePathCompleter fileCompleter(navigator);
   return fileCompleter.getSuggestions(partial);
   
   // For command name completions
   CommandCompleter cmdCompleter(terminal.getCommands());
   return cmdCompleter.getSuggestions(partial);
   ```

**See also**: The built-in commands implement auto completion - check [src/commands/cd.h](src/commands/cd.h), [src/commands/cat.h](src/commands/cat.h), and [src/commands/ls.h](src/commands/ls.h) for examples.

## Testing

### Writing Tests

- Add unit tests for new functionality
- Use mock implementations for hardware dependencies
- Test edge cases and error conditions
- Ensure tests pass on all target platforms

### Test Structure

```cpp
#include <unity.h>
#include "YourClass.h"

void setUp(void) {
    // Setup before each test
}

void tearDown(void) {
    // Cleanup after each test
}

void test_your_feature(void) {
    YourClass obj;
    TEST_ASSERT_EQUAL(expected, obj.method());
}

void process_tests() {
    UNITY_BEGIN();
    RUN_TEST(test_your_feature);
    UNITY_END();
}
```

### Running Tests

```bash
# Test on native (fastest)
pio test -e native

# Test on ESP32
pio test -e esp32s3_espressif

# Test specific test suite
pio test -e native -f test_core/test_ETTypes
```

## Documentation

### Code Documentation

- Add **docstrings** for public APIs
- Include **usage examples** in comments
- Document **parameters and return values**
- Explain **complex algorithms**

### README Updates

When adding new features:

- Update the **Features** section
- Add to **Built-in Commands** table if applicable
- Update **API Reference** if needed
- Add **usage examples**

## Commit Messages

Write clear, concise commit messages:

```txt
Add support for command history

- Implement history buffer with configurable size
- Add up/down arrow key navigation
- Update tests for history functionality
```

### Format

```txt
<type>: <short summary>

<optional detailed description>

<optional footer>
```

**Types**: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

## Review Process

1. Maintainer will review your pull request
2. Feedback may be provided for changes
3. Once approved, PR will be merged
4. Changes will be included in next release

## Release Process

Releases follow [Semantic Versioning](https://semver.org/):

- **Major** (1.0.0): Breaking changes
- **Minor** (0.1.0): New features (backward compatible)
- **Patch** (0.0.1): Bug fixes (backward compatible)

## Questions?

Feel free to open an issue for:

- Questions about contributing
- Clarifications on guidelines
- Help with development setup
- General project questions

## Recognition

Contributors will be acknowledged in:

- CHANGELOG.md
- GitHub contributors page
- Release notes (for significant contributions)

Thank you for contributing to EmbeddedTerminal! 🚀

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
- Use platform-specific code only in HAL layer

### Example

```cpp
// Good: Platform-independent
ETString processInput(const ETString &input) {
    ETString result = input.trim();
    return result;
}

// Avoid: Platform-specific
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
pio test -e native -f test_ETTypes
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

```
Add support for command history

- Implement history buffer with configurable size
- Add up/down arrow key navigation
- Update tests for history functionality
```

### Format

```
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

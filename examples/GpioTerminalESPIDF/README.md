# GPIO Terminal Example (ESP-IDF)

This example demonstrates secure GPIO control using the EmbeddedTerminal library on ESP32 ESP-IDF framework.

## Features

- Secure GPIO access with compile-time policy configuration
- Password-protected administration
- Forced exclusions to protect critical pins (bootloader, UART, etc.)
- Per-operation access control (read/write/mode/exclude/include)
- Cross-platform pin identifier support (GPIO2, 2, PA5)

## Build and Flash

```bash
idf.py build
idf.py flash monitor
```

## Configuration

Security settings are configured in `main/CMakeLists.txt`:

```cmake
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    ET_GPIO_ENABLE=1
    ET_GPIO_ALLOWED_PINS="GPIO2,4,5,12,13,14,15"
    ET_GPIO_FORCED_EXCLUSIONS="GPIO0:r,w,m,e,i;GPIO45:r,w,m,i;GPIO46:r,w,m,i"
    ET_GPIO_DEFAULT_POLICY=1
    ET_GPIO_ADMIN_HASH="0xbf1075ac"
)
```

### Configuration Options

- `ET_GPIO_ENABLE`: Enable GPIO support (0/1)
- `ET_GPIO_ALLOWED_PINS`: CSV allowlist used by policy (optional; empty means all detected board pins)
- `ET_GPIO_FORCED_EXCLUSIONS`: Semicolon-separated exclusion rules (format: "pin:flags")
- `ET_GPIO_DEFAULT_POLICY`: 0=allow unlisted pins, 1=deny unlisted pins
- `ET_GPIO_ADMIN_HASH`: FNV-1a 32-bit hash of admin password (hex format)

### Exclusion Rule Flags

- `r` - Deny read operations
- `w` - Deny write operations
- `m` - Deny mode changes
- `e` - Protected (requires admin auth to modify exclusions)
- `i` - Immutable (cannot be included, even by admin)

## Usage Examples

```
> gpio list
Available pins:
  GPIO2, GPIO4, GPIO5, GPIO12, GPIO13, GPIO14, GPIO15
Excluded pins:
  GPIO0 (forced: read, write, mode, exclude, include)
  GPIO45 (forced: read, write, mode, include)
  GPIO46 (forced: read, write, mode, include)

> gpio mode 2 output
Pin GPIO2 mode set to output

> gpio write 2 1
Pin GPIO2 set to HIGH

> gpio read 2
Pin GPIO2: HIGH (1)

> gpio deny 5 r,w
Pin GPIO5 excluded from: read, write

> gpio auth mypassword
Authentication successful

> gpio allow 5
Pin GPIO5 allow rule applied
```

## Generating Admin Hash

To generate the admin password hash, use this helper sketch:

```cpp
#include <hal/common/CompileTimeGpioAuth.h>

void setup() {
    Serial.begin(115200);
    ETString hash = CompileTimeGpioAuth::hashPasswordHex("mypassword");
    Serial.println("Admin hash: " + hash);
    // Output: Admin hash: 0xbf1075ac
}
```

## Security Notes

1. **Forced Exclusions**: Pins marked with `i` flag cannot be included, protecting critical system pins
2. **Protected Exclusions**: Pins marked with `e` flag require authentication to modify
3. **Session Auth**: Authentication with `gpio auth` is session-only (resets on reboot)
4. **No Plaintext**: Admin password is stored as compile-time hash, never in plaintext
5. **Default Deny**: With `ET_GPIO_DEFAULT_POLICY=1`, operations on pins outside `ET_GPIO_ALLOWED_PINS` are blocked

/*
 * GPIO Terminal Example (Arduino Framework)
 *
 * This example demonstrates secure GPIO control using the EmbeddedTerminal
 * library on ESP32 Arduino. It showcases compile-time security policies,
 * password-protected administration, and forced exclusions.
 *
 * GPIO Commands available:
 * - gpio list      : Show board pins and policy status
 * - gpio policy    : Show current security policy
 * - gpio read <pin>      : Read digital value from pin
 * - gpio write <pin> <0|1> : Write digital value to pin
 * - gpio mode <pin> <mode> : Set pin mode (input/output/input_pullup/input_pulldown)
 * - gpio deny <pin> [ops]    : Deny operations for pin
 * - gpio allow <pin> [ops]   : Allow operations for pin
 * - gpio excluded            : List all excluded pins
 * - gpio auth <password>     : Authenticate for protected operations
 *
 * Security Configuration (platformio.ini):
 * - Policy allowlist: GPIO2, 4, 5, 12, 13, 14, 15
 * - Forced exclusions: GPIO0, 45, 46 (bootloader/UART)
 * - Default policy: Deny (blocks operations on pins outside allowlist)
 * - Admin password: Compile-time hash verification
 *
 * Usage:
 * 1. Configure build_flags in platformio.ini (see below)
 * 2. Upload this sketch to your ESP32 board
 * 3. Open Serial Monitor (115200 baud)
 * 4. Try commands like:
 *    > gpio list
 *    > gpio read 2
 *    > gpio mode 5 output
 *    > gpio write 5 1
 *    > gpio deny 5 r,w
 *    > gpio auth mypassword
 *
 * Build Configuration (platformio.ini):
 *
 * [env:esp32]
 * platform = espressif32
 * framework = arduino
 * board = esp32-s3-devkitc-1
 *
 * build_flags =
 *     -DET_GPIO_ENABLE=1
 *     -DET_GPIO_ALLOWED_PINS=\"GPIO2,4,5,12,13,14,15\"
 *     -DET_GPIO_FORCED_EXCLUSIONS=\"GPIO0:r,w,m,e,i;GPIO45:r,w,m,i;GPIO46:r,w,m,i\"
 *     -DET_GPIO_DEFAULT_POLICY=1
 *     -DET_GPIO_ADMIN_HASH=\"0xbf1075ac\"
 */
#define ET_GPIO_ENABLE 1
#include <Arduino.h>
#include <Terminal.h>
#include <BuiltinCommandFactory.h>
#include <BuiltinCommandFlags.h>
#include <hal/common/DefaultGpioSupport.h>

using namespace EmbeddedTerminal;

// Create terminal instance
Terminal term(Serial);

// Create factory instance (owns built-in commands)
BuiltinCommandFactory factory;

#if ET_GPIO_ENABLE
DefaultGpioSupport gpioSupport;
#endif

void setup()
{
    // Initialize Serial communication
    Serial.begin(115200);
    delay(1000); // Wait for Serial to initialize

    // Clear screen and show welcome message
    Serial.println("\n\n");
    Serial.println("=============================================");
    Serial.println("  EmbeddedTerminal - GPIO Control (Arduino)");
    Serial.println("=============================================");
    Serial.println();

    // Check if GPIO support is available
#if ET_GPIO_ENABLE
    IGpioInterface *gpio = gpioSupport.gpio();
    if (gpioSupport.available() && gpio != nullptr)
    {
        Serial.println("[INFO] GPIO support enabled");
        Serial.println("[INFO] Registering GPIO commands...");

        // Register GPIO commands with security policy
        factory.registerGpioCommands(term,
                                     *gpio,
                                     gpioSupport.policy(),
                                     gpioSupport.auth());

        Serial.println("[INFO] GPIO commands registered successfully");
        Serial.println("[INFO] gpio list shows board pins; policy may still block operations");
        Serial.println();

        // Show security configuration
        Serial.println("Security Configuration:");
        Serial.println("  - Default policy: " + ETString(ET_GPIO_DEFAULT_POLICY ? "DENY" : "ALLOW"));
#ifdef ET_GPIO_ALLOWED_PINS
        Serial.println("  - Allowed pins: " + ETString(ET_GPIO_ALLOWED_PINS));
#endif
#ifdef ET_GPIO_FORCED_EXCLUSIONS
        Serial.println("  - Forced exclusions: " + ETString(ET_GPIO_FORCED_EXCLUSIONS));
#endif
#ifdef ET_GPIO_ADMIN_HASH
        Serial.println("  - Admin authentication: ENABLED");
#else
        Serial.println("  - Admin authentication: DISABLED");
#endif
        Serial.println();
    }
    else
    {
        Serial.println("[WARN] GPIO support not available (check build configuration)");
        Serial.println();
    }
#else
    Serial.println("[WARN] GPIO support disabled (ET_GPIO_ENABLE not defined)");
    Serial.println("[INFO] Add -DET_GPIO_ENABLE=1 to build_flags in platformio.ini");
    Serial.println();
#endif

    // Register help command
    factory.registerHelpCommand(term);
    factory.registerScriptCommand(term);

    // Show usage instructions
    Serial.println("Terminal ready! Available commands:");
    Serial.println("  help         - List all commands");
    Serial.println("  script ...   - Run scripting expressions and script files");
#if ET_GPIO_ENABLE
    Serial.println("  gpio list    - Show board pins and policy status");
    Serial.println("  gpio policy  - Show security policy");
    Serial.println("  gpio read <pin>      - Read pin value");
    Serial.println("  gpio write <pin> <0|1> - Write pin value");
    Serial.println("  gpio mode <pin> <mode> - Set pin mode");
    Serial.println();
    Serial.println("Example session:");
    Serial.println("  > gpio list");
    Serial.println("  > gpio mode 2 output");
    Serial.println("  > gpio write 2 1");
    Serial.println("  > gpio read 2");
#endif
    Serial.println();
    Serial.print("> ");
}

void loop()
{
    // Process terminal input
    term.loop();
}

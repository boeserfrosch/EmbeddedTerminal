/*
 * GPIO Terminal Example (ESP-IDF Framework)
 *
 * This example demonstrates secure GPIO control using the EmbeddedTerminal
 * library on ESP32 ESP-IDF. It showcases compile-time security policies,
 * password-protected administration, and forced exclusions.
 *
 * GPIO Commands available:
 * - gpio list      : Show allowed and excluded pins
 * - gpio policy    : Show current security policy
 * - gpio read <pin>      : Read digital value from pin
 * - gpio write <pin> <0|1> : Write digital value to pin
 * - gpio mode <pin> <mode> : Set pin mode (input/output/input_pullup/input_pulldown)
 * - gpio deny <pin> [ops]    : Deny operations for pin
 * - gpio allow <pin> [ops]   : Allow operations for pin
 * - gpio excluded            : List all excluded pins
 * - gpio auth <password>     : Authenticate for protected operations
 *
 * Security Configuration (CMakeLists.txt):
 * - Allowed pins: GPIO2, 4, 5, 12, 13, 14, 15
 * - Forced exclusions: GPIO0, 45, 46 (bootloader/UART)
 * - Default policy: Deny (only allowed pins accessible)
 * - Admin password: Compile-time hash verification
 *
 * Usage:
 * 1. Configure CMakeLists.txt with security flags (see below)
 * 2. Build and flash: idf.py build flash monitor
 * 3. Try commands like:
 *    > gpio list
 *    > gpio read 2
 *    > gpio mode 5 output
 *    > gpio write 5 1
 *    > gpio deny 5 r,w
 *    > gpio auth mypassword
 *
 * Build Configuration (CMakeLists.txt):
 *
 * idf_component_register(SRCS "main.cpp"
 *                        INCLUDE_DIRS ".")
 *
 * target_compile_definitions(${COMPONENT_LIB} PRIVATE
 *     ET_GPIO_ENABLE=1
 *     ET_GPIO_ALLOWED_PINS="GPIO2,4,5,12,13,14,15"
 *     ET_GPIO_FORCED_EXCLUSIONS="GPIO0:r,w,m,e,i;GPIO45:r,w,m,i;GPIO46:r,w,m,i"
 *     ET_GPIO_DEFAULT_POLICY=1
 *     ET_GPIO_ADMIN_HASH="0xbf1075ac"
 * )
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include <Terminal.h>
#include <commands/BuiltinCommands.h>
#include <hal/common/DefaultGpioSupport.h>
#include <hal/espidf/ESPIDFTerminalStream.h>

using namespace EmbeddedTerminal;

// Create terminal stream wrapper for UART
ESPIDFTerminalStream termStream(UART_NUM_0);

// Create terminal instance
Terminal term(termStream);

#if ET_GPIO_ENABLE
DefaultGpioSupport gpioSupport;
#endif

extern "C" void app_main(void)
{
    // Initialize UART
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);

    // Clear screen and show welcome message
    printf("\n\n");
    printf("=============================================\n");
    printf("  EmbeddedTerminal - GPIO Control (ESP-IDF)\n");
    printf("=============================================\n");
    printf("\n");

    // Check if GPIO support is available
#if ET_GPIO_ENABLE
    IGpioInterface *gpio = gpioSupport.gpio();
    if (gpioSupport.available() && gpio != nullptr)
    {
        printf("[INFO] GPIO support enabled\n");
        printf("[INFO] Registering GPIO commands...\n");

        static cmd::gpio gpioCommand(*gpio,
                                     gpioSupport.policy(),
                                     gpioSupport.auth());
        term.registerCommand("gpio", &gpioCommand);

        printf("[INFO] GPIO commands registered successfully\n");
        printf("[INFO] gpio list shows board pins; policy may still block operations\n");
        printf("\n");

        // Show security configuration
        printf("Security Configuration:\n");
        printf("  - Default policy: %s\n", ET_GPIO_DEFAULT_POLICY ? "DENY" : "ALLOW");
#ifdef ET_GPIO_ALLOWED_PINS
        printf("  - Allowed pins: %s\n", ET_GPIO_ALLOWED_PINS);
#endif
#ifdef ET_GPIO_FORCED_EXCLUSIONS
        printf("  - Forced exclusions: %s\n", ET_GPIO_FORCED_EXCLUSIONS);
#endif
#ifdef ET_GPIO_ADMIN_HASH
        printf("  - Admin authentication: ENABLED\n");
#else
        printf("  - Admin authentication: DISABLED\n");
#endif
        printf("\n");
    }
    else
    {
        printf("[WARN] GPIO support not available (check build configuration)\n");
        printf("\n");
    }
#else
    printf("[WARN] GPIO support disabled (ET_GPIO_ENABLE not defined)\n");
    printf("[INFO] Add ET_GPIO_ENABLE=1 to CMakeLists.txt\n");
    printf("\n");
#endif

    static cmd::help helpCommand(term);
    term.registerCommand("help", &helpCommand);

    // Show usage instructions
    printf("Terminal ready! Available commands:\n");
    printf("  help         - List all commands\n");
#if ET_GPIO_ENABLE
    printf("  gpio list    - Show board pins and policy status\n");
    printf("  gpio policy  - Show security policy\n");
    printf("  gpio read <pin>      - Read pin value\n");
    printf("  gpio write <pin> <0|1> - Write pin value\n");
    printf("  gpio mode <pin> <mode> - Set pin mode\n");
    printf("\n");
    printf("Example session:\n");
    printf("  > gpio list\n");
    printf("  > gpio mode 2 output\n");
    printf("  > gpio write 2 1\n");
    printf("  > gpio read 2\n");
#endif
    printf("\n");
    printf("> ");

    // Main loop
    while (1)
    {
        term.loop();
        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to prevent watchdog triggers
    }
}

// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif

#include "commands/gpio.h"
#include "../Mocks/MockGpioInterface.h"
#include "../Mocks/MockGpioPolicy.h"
#include "../Mocks/MockGpioAuth.h"
#include "../Mocks/MockStream.h"
#include "../Mocks/CommandRuntimeTestUtils.h"

using namespace EmbeddedTerminal;

void setUp(void) {}
void tearDown(void) {}

void test_gpio_list_contains_known_pins(void)
{
    MockGpioInterface gpioHw;
    MockGpioPolicy policy;
    cmd::gpio gpioCmd(gpioHw, policy);

    ETString out = gpioCmd.trigger("gpio", "list");
    TEST_ASSERT_TRUE(out.find("GPIO2") != ETString::npos);
    TEST_ASSERT_TRUE(out.find("GPIO5") != ETString::npos);
}

void test_gpio_read_and_write(void)
{
    MockGpioInterface gpioHw;
    MockGpioPolicy policy;
    cmd::gpio gpioCmd(gpioHw, policy);

    ETString writeResult = gpioCmd.trigger("gpio", "write 2 1");
    TEST_ASSERT_TRUE(writeResult.find("OK") != ETString::npos);

    ETString readResult = gpioCmd.trigger("gpio", "read 2");
    TEST_ASSERT_TRUE(readResult.find("GPIO2=1") != ETString::npos);
}

void test_gpio_forced_exclusion_cannot_be_allowed(void)
{
    MockGpioInterface gpioHw;
    MockGpioPolicy policy;

    GpioExclusionRule forced;
    forced.pinId = "GPIO2";
    forced.forced = true;
    forced.denyRead = true;
    forced.denyWrite = true;
    forced.denyMode = true;
    ETString reason;
    policy.addExclusion(forced, reason);

    cmd::gpio gpioCmd(gpioHw, policy);
    ETString includeResult = gpioCmd.trigger("gpio", "allow 2");

    TEST_ASSERT_TRUE(includeResult.find("forced") != ETString::npos);
}

void test_gpio_protected_exclusion_requires_authentication(void)
{
    MockGpioInterface gpioHw;
    MockGpioPolicy policy;
    MockGpioAuth auth;
    cmd::gpio gpioCmd(gpioHw, policy, &auth);

    MockStream stream;
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);

    CommandInvocation protectedExclude{"gpio", "deny 2 write --protected", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult firstResult = gpioCmd.execute(protectedExclude);
    TEST_ASSERT_NOT_EQUAL(0, firstResult.exitCode);
    TEST_ASSERT_TRUE(stream.stderrBuffer.find("Authentication required") != ETString::npos);

    CommandInvocation authInvoke{"gpio", "auth secret", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult authResult = gpioCmd.execute(authInvoke);
    TEST_ASSERT_EQUAL(0, authResult.exitCode);

    CommandResult secondResult = gpioCmd.execute(protectedExclude);
    TEST_ASSERT_EQUAL(0, secondResult.exitCode);
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_gpio_list_contains_known_pins);
    RUN_TEST(test_gpio_read_and_write);
    RUN_TEST(test_gpio_forced_exclusion_cannot_be_allowed);
    RUN_TEST(test_gpio_protected_exclusion_requires_authentication);
    UNITY_END();
}

#if (defined(ESP_PLATFORM) || defined(ESP32)) && not defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
extern "C" void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(4000));
    process_tests();
}
#elif defined(ARDUINO)
void setup()
{
    delay(2500);
    process_tests();
}
void loop() {}
#else
int main()
{
    process_tests();
    return 0;
}
#endif
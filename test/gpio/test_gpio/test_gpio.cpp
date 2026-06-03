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
#include "../../Mocks/MockGpioInterface.h"
#include "../../Mocks/MockGpioPolicy.h"
#include "../../Mocks/MockGpioAuth.h"
#include "../../commands/utils.h"

using namespace EmbeddedTerminal;

void setUp(void) {}
void tearDown(void) {}

void test_gpio_list_contains_known_pins(void)
{
    MockGpioInterface gpioHw;
    MockGpioPolicy policy;
    cmd::gpio gpioCmd(gpioHw, policy);

    auto iHandle = TestCommandInvocationHandle("gpio", {"list"});
    auto result = gpioCmd.invoke(iHandle.invocation);
    TEST_ASSERT_EQUAL(0, result.exitCode);

    TEST_ASSERT_TRUE(iHandle.output.contains("GPIO2"));
    TEST_ASSERT_TRUE(iHandle.output.contains("GPIO5"));
}

void test_gpio_read_and_write(void)
{
    MockGpioInterface gpioHw;
    MockGpioPolicy policy;
    cmd::gpio gpioCmd(gpioHw, policy);

    auto iHandle = TestCommandInvocationHandle("gpio", {"write", "2", "1"});
    CommandResult writeResult = gpioCmd.invoke(iHandle.invocation);
    TEST_ASSERT_EQUAL(0, writeResult.exitCode);
    TEST_ASSERT_TRUE(iHandle.output.contains("OK"));

    auto iHandle2 = TestCommandInvocationHandle("gpio", {"read", "2"});
    CommandResult readResult = gpioCmd.invoke(iHandle2.invocation);
    TEST_ASSERT_TRUE(iHandle2.output.contains("GPIO2=1"));
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

    auto iHandle = TestCommandInvocationHandle("gpio", {"allow", "2"});
    CommandResult result = gpioCmd.invoke(iHandle.invocation);
    TEST_ASSERT_NOT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(iHandle.error.contains("forced"));
}

void test_gpio_protected_exclusion_requires_authentication(void)
{
    MockGpioInterface gpioHw;
    MockGpioPolicy policy;
    MockGpioAuth auth;
    cmd::gpio gpioCmd(gpioHw, policy, &auth);

    auto iHandle = TestCommandInvocationHandle("gpio", {"deny", "2", "write", "--protected"});
    CommandResult excludeResult = gpioCmd.invoke(iHandle.invocation);
    TEST_ASSERT_NOT_EQUAL(0, excludeResult.exitCode);
    TEST_ASSERT_TRUE(iHandle.error.contains("Authentication required"));

    auto authInvoke = TestCommandInvocationHandle("gpio", {"auth", "secret"});
    TEST_ASSERT_FALSE(authInvoke.variables.find("gpio__authenticated") != authInvoke.variables.end());
    CommandResult authResult = gpioCmd.invoke(authInvoke.invocation);
    TEST_ASSERT_EQUAL(0, authResult.exitCode);
    TEST_ASSERT_TRUE(authInvoke.output.contains("Authenticated"));
    TEST_ASSERT_TRUE(authInvoke.variables.find("gpio__authenticated") != authInvoke.variables.end());

    iHandle.invocation.context.variables["gpio__authenticated"] = "1";
    iHandle.invocation.context.variables["gpio__auth_until_ms"] = authInvoke.invocation.context.variables["gpio__auth_until_ms"];
    CommandResult secondResult = gpioCmd.invoke(iHandle.invocation);
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
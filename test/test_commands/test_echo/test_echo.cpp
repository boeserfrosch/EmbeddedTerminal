// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif

#include "commands/echo.h"
#include "../../Mocks/MockStream.h"
#include "../../Mocks/CommandRuntimeTestUtils.h"

using namespace EmbeddedTerminal;

void setUp(void) {}
void tearDown(void) {}

void test_echo_trigger_with_text(void)
{
    cmd::echo echoCmd;
    ETString result = echoCmd.trigger("echo", "hello world");
    TEST_ASSERT_EQUAL_STRING("hello world\n", result.c_str());
}

void test_echo_trigger_empty_text(void)
{
    cmd::echo echoCmd;
    ETString result = echoCmd.trigger("echo", "");
    TEST_ASSERT_EQUAL_STRING("\n", result.c_str());
}

void test_echo_usage(void)
{
    cmd::echo echoCmd;
    ETString usage = echoCmd.usage("echo");
    TEST_ASSERT_TRUE(usage.find("Print the specified text") != ETString::npos);
}

void test_echo_execute_writes_stdout(void)
{
    cmd::echo echoCmd;

    MockStream stream;
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{"echo", "embedded terminal", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = echoCmd.execute(invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("embedded terminal") != ETString::npos);
    TEST_ASSERT_TRUE(stream.stderrBuffer.empty());
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_echo_trigger_with_text);
    RUN_TEST(test_echo_trigger_empty_text);
    RUN_TEST(test_echo_usage);
    RUN_TEST(test_echo_execute_writes_stdout);
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

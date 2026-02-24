// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "commands/df.h"
#include "../Mocks/MockFileSystem.h"
#include "../Mocks/MockStream.h"
#include "../Mocks/CommandRuntimeTestUtils.h"
#include "DirectoryNavigator.h"

using namespace EmbeddedTerminal;
void setUp(void) {}
void tearDown(void) {}

void test_df_trigger_basic(void)
{
    MockFileSystem FS;
    FS._capacity = 1024 * 1024 * 8; // 8MB card
    cmd::df df(FS);
    ETString keyword = "df";
    ETString additional = "";
    ETString result = df.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("Filesystem") != ETString::npos || result.find("Size") != ETString::npos);
    TEST_ASSERT_TRUE(result.find("Size 8.0 MB") != ETString::npos); // Check for 8MB size in MB
}

void test_df_trigger_with_path(void)
{
    MockFileSystem FS;
    FS._capacity = 1024 * 1024 * 8; // 8MB card
    cmd::df df(FS);
    ETString keyword = "df";
    ETString additional = "/somepath";
    ETString result = df.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("Filesystem") != ETString::npos || result.find("Size") != ETString::npos);
}

void test_df_usage(void)
{
    MockFileSystem FS;
    FS._capacity = 1024 * 1024 * 8; // 8MB card
    cmd::df df(FS);
    ETString keyword = "df";
    ETString result = df.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Usage") != ETString::npos || result.find("Show disk usage") != ETString::npos);
}

void test_df_trigger_edge_cases(void)
{
    MockFileSystem FS;
    FS._capacity = 1024 * 1024 * 8; // 8MB card
    cmd::df df(FS);
    ETString keyword = "df";
    ETString additional = "   "; // whitespace
    ETString result = df.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("Filesystem") != ETString::npos || result.find("Size") != ETString::npos);

    additional = "";
    result = df.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("Filesystem") != ETString::npos || result.find("Size") != ETString::npos);
}

void test_df_trigger_output_format(void)
{
    MockFileSystem FS;
    FS._capacity = 1024 * 1024 * 8; // 8MB card
    cmd::df df(FS);
    ETString keyword = "df";
    ETString additional = "";
    ETString result = df.trigger(keyword, additional);
    // Check for expected columns
    TEST_ASSERT_TRUE(result.find("Used") != ETString::npos);
    TEST_ASSERT_TRUE(result.find("Free") != ETString::npos);
}

void test_df_execute_writes_stdout(void)
{
    MockFileSystem FS;
    FS._capacity = 1024 * 1024 * 8;
    cmd::df df(FS);

    MockStream stream;
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{"df", "", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = df.execute(invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("Size") != ETString::npos);
    TEST_ASSERT_TRUE(stream.stderrBuffer.empty());
}

void process_tests()
{

    UNITY_BEGIN();
    RUN_TEST(test_df_trigger_basic);
    RUN_TEST(test_df_trigger_with_path);
    RUN_TEST(test_df_usage);
    RUN_TEST(test_df_trigger_edge_cases);
    RUN_TEST(test_df_trigger_output_format);
    RUN_TEST(test_df_execute_writes_stdout);
    UNITY_END();
}

#if (defined(ESP_PLATFORM) || defined(ESP32)) && not defined(ARDUINO)
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

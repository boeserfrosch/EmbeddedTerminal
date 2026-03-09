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
#include "../Mocks/native/MockFileSystem.h"
#include "../Mocks/MockStream.h"
#include "../Mocks/CommandRuntimeTestUtils.h"
#include "DirectoryNavigator.h"
#include "StorageSystem.h"
#include "../Mocks/native/MockStorageMedia.h"

using namespace EmbeddedTerminal;
IStorageSystem *storage = nullptr;

void setUp(void)
{
    storage = new StorageSystem();
    auto media = new MockStorageMedia("mock", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    auto media2 = new MockStorageMedia("mock2", true, 1024 * 1024 * 8, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storage->mountMedia(media, "/");
    storage->mountMedia(media2, "/mock2");
}
void tearDown(void)
{
    auto medias = storage->media();
    for (auto media : medias)
    {
        storage->unmountMedia(media->name());
        delete media;
    }
    delete storage;
    storage = nullptr;
}

void test_df_trigger_basic(void)
{
    cmd::df df(*storage);
    ETString keyword = "df";
    ETString additional = "";
    ETString result = df.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("Filesystem") != ETString::npos || result.find("Size") != ETString::npos);
    TEST_ASSERT_TRUE(result.find("Size 8.0 MB") != ETString::npos); // Check for 8MB size in MB
}

void test_df_trigger_with_path(void)
{
    cmd::df df(*storage);
    ETString keyword = "df";
    ETString additional = "/somepath";
    ETString result = df.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("Filesystem") != ETString::npos || result.find("Size") != ETString::npos);
}

void test_df_usage(void)
{
    cmd::df df(*storage);
    ETString keyword = "df";
    ETString result = df.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Usage") != ETString::npos || result.find("Show disk usage") != ETString::npos);
}

void test_df_trigger_edge_cases(void)
{
    cmd::df df(*storage);
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
    cmd::df df(*storage);
    ETString keyword = "df";
    ETString additional = "";
    ETString result = df.trigger(keyword, additional);
    // Check for expected columns
    TEST_ASSERT_TRUE(result.find("Used") != ETString::npos);
    TEST_ASSERT_TRUE(result.find("Free") != ETString::npos);
}

void test_df_execute_writes_stdout(void)
{
    cmd::df df(*storage);

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

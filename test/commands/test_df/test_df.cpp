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
#include "../../Mocks/native/MockFileSystem.h"
#include "../../Mocks/CommandRuntimeTestUtils.h"
#include "DirectoryNavigator.h"
#include "StorageSystem.h"
#include "../../Mocks/native/MockStorageMedia.h"
#include "../utils.h"
#include <memory>

using namespace EmbeddedTerminal;
IStorageSystem *storage = nullptr;
static std::shared_ptr<MockStorageMedia> media;
static std::shared_ptr<MockStorageMedia> media2;

void setUp(void)
{
    storage = new StorageSystem();
    media = std::make_shared<MockStorageMedia>("mock", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    media2 = std::make_shared<MockStorageMedia>("mock2", true, 1024 * 1024 * 8, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storage->mountMedia(media, "/");
    storage->mountMedia(media2, "/mock2");
}
void tearDown(void)
{
    if (media2)
    {
        storage->unmountMedia(media2->name());
        media2.reset();
    }
    if (media)
    {
        storage->unmountMedia(media->name());
        media.reset();
    }
    delete storage;
    storage = nullptr;
}

void test_df_usage(void)
{
    cmd::df df(*storage);
    ETString keyword = "df";
    ETString result = df.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Usage") != ETString::npos || result.find("Show disk usage") != ETString::npos);
}

void test_df_edge_cases(void)
{
    cmd::df df(*storage);
    ETString keyword = "df";

    TestCommandInvocationHandle iHandle(keyword, {});
    ;
    CommandResult result = df.invoke(iHandle.invocation);
    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(iHandle.output.contains("Size"));
    TEST_ASSERT_TRUE(iHandle.output.contains("Filesystem"));
    TEST_ASSERT_TRUE(iHandle.error.empty());
}

void test_df_output_format(void)
{
    cmd::df df(*storage);
    ETString keyword = "df";
    ETString additional = "";
    TestCommandInvocationHandle iHandle(keyword, {additional});
    ;
    CommandResult cmdResult = df.invoke(iHandle.invocation);
    // Check for expected columns
    TEST_ASSERT_TRUE(iHandle.output.contains("Used"));
    TEST_ASSERT_TRUE(iHandle.output.contains("Free"));
}

void test_df_writes_stdout(void)
{
    cmd::df df(*storage);

    ETMap<ETString, ETString> vars;
    TestCommandInvocationHandle invocationHandle("df", {});

    CommandResult result = df.invoke(invocationHandle.invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(invocationHandle.output.contains("Size"));
    TEST_ASSERT_TRUE(invocationHandle.error.empty());
}

void process_tests()
{

    UNITY_BEGIN();
    RUN_TEST(test_df_usage);
    RUN_TEST(test_df_edge_cases);
    RUN_TEST(test_df_output_format);
    RUN_TEST(test_df_writes_stdout);
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

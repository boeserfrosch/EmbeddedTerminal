// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif

#include "commands/wc.h"
#include "StorageSystem.h"
#include "../../Mocks/native/MockFileSystem.h"
#include "../../Mocks/native/MockStorageMedia.h"
#include "../utils.h"
#include <memory>

using namespace EmbeddedTerminal;

IStorageSystem *storage = nullptr;
DirectoryNavigator *dir = nullptr;
static std::shared_ptr<MockStorageMedia> media;

void setUp(void)
{
    storage = new StorageSystem();
    media = std::make_shared<MockStorageMedia>("mock", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storage->mountMedia(media, "/");
    dir = new DirectoryNavigator(storage);
}

void tearDown(void)
{
    if (media)
    {
        storage->unmountMedia(media->name());
        media.reset();
    }
    delete dir;
    dir = nullptr;
    delete storage;
    storage = nullptr;
}

void test_wc_trigger_file_counts(void)
{
    ETFile file = storage->open("/sample.txt", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(file.isOpen());
    TEST_ASSERT_TRUE(file.writeAll("one two\nthree\n"));
    file.close();

    cmd::wc wcCmd(*dir);
    TestCommandInvocationHandle iHandle("wc", {"sample.txt"});
    ;
    CommandResult result = wcCmd.invoke(iHandle.invocation);
    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(iHandle.output.contains("sample.txt:\t2\t3\t14\n"));
}

void test_wc_trigger_missing_file(void)
{
    cmd::wc wcCmd(*dir);
    TestCommandInvocationHandle iHandle("wc", {"missing.txt"});
    ;
    CommandResult result = wcCmd.invoke(iHandle.invocation);
    TEST_ASSERT_EQUAL(1, result.exitCode);

    TEST_ASSERT_TRUE(iHandle.error.contains("did not exist"));
}

void test_wc_usage(void)
{
    cmd::wc wcCmd(*dir);
    ETString usage = wcCmd.usage("wc");
    TEST_ASSERT_TRUE(usage.find("line, word and byte") != ETString::npos);
}

void test_wc_execute_reads_stdin_when_no_file(void)
{
    cmd::wc wcCmd(*dir);

    TestCommandInvocationHandle iHandle("wc");
    ;
    iHandle.input.print("hello world\nthis is a test\n");
    CommandResult result = wcCmd.invoke(iHandle.invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_EQUAL(CommandExecutionState::Completed, result.state);
    TEST_ASSERT_TRUE(iHandle.output.contains("Lines\tWords\tBytes\n")); // Check if header is printed
    TEST_ASSERT_TRUE(iHandle.output.contains("2\t6\t27\n"));
    TEST_ASSERT_TRUE(iHandle.error.empty());
}

void test_wc_execute_file_counts(void)
{
    ETFile file = storage->open("/file.txt", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(file.isOpen());
    TEST_ASSERT_TRUE(file.writeAll("hello world\n"));
    file.close();

    cmd::wc wcCmd(*dir);

    TestCommandInvocationHandle iHandle("wc", {"file.txt"});
    ;
    CommandResult result = wcCmd.invoke(iHandle.invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_EQUAL(CommandExecutionState::Completed, result.state);
    // Check if header is printed
    TEST_ASSERT_TRUE(iHandle.output.contains("File\tLines\tWords\tBytes\n"));
    TEST_ASSERT_TRUE(iHandle.output.contains("file.txt:\t1\t2\t12\n"));
    TEST_ASSERT_TRUE(iHandle.error.empty());
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_wc_trigger_file_counts);
    RUN_TEST(test_wc_trigger_missing_file);
    RUN_TEST(test_wc_usage);
    RUN_TEST(test_wc_execute_reads_stdin_when_no_file);
    RUN_TEST(test_wc_execute_file_counts);
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

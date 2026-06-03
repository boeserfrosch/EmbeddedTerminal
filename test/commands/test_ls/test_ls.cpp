// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "commands/ls.h"
#include "../../Mocks/native/MockFileSystem.h"
#include "../utils.h"
#include "DirectoryNavigator.h"
#include "StorageSystem.h"
#include "../../Mocks/native/MockStorageMedia.h"
#include <memory>

IStorageSystem *storage = nullptr;
DirectoryNavigator *dir = nullptr;
static std::shared_ptr<MockStorageMedia> media;

void setUp(void)
{
    storage = new StorageSystem();
    media = std::make_shared<MockStorageMedia>("mock", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storage->mountMedia(media, "/");
    dir = new DirectoryNavigator(storage);
    storage->open("/file.txt", "w", true).writeAll("hello");
    storage->mkdir("/dir1");
    storage->mkdir("/dir2");
    storage->open("/dir2/foo.txt", "w", true).writeAll("bar");
    storage->open("/dir3/foo.txt", "w", true).writeAll("bar");
    storage->open("/dir3/bar.txt", "w", true).writeAll("bar");
    storage->open("/dir3/baz.txt", "w", true).writeAll("bar");
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

void test_ls_valid_directory(void)
{
    cmd::ls ls(*dir);
    ETString keyword = "ls";
    TestCommandInvocationHandle iHandle("ls", {"-l", "dir2"});
    ;
    CommandResult result = ls.invoke(iHandle.invocation);
    TEST_ASSERT_EQUAL(0, result.exitCode);
    // Should show foo.txt with details (simulate long listing)
    TEST_ASSERT_TRUE(iHandle.output.contains("foo.txt"));
    TEST_ASSERT_FALSE(iHandle.output.contains("bar")); // Content should not be shown
    // Check for typical long listing info (e.g., size, type)
    TEST_ASSERT_TRUE(iHandle.output.contains("Bytes"));
}

void test_ls_edge_cases(void)
{
    cmd::ls ls(*dir);
    ETString keyword = "ls";
    TestCommandInvocationHandle iHandle("ls");
    ;
    CommandResult result = ls.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains("file.txt"));
}

void test_ls_output_format(void)
{
    cmd::ls ls(*dir);
    ETString keyword = "ls";
    ETString arg = "dir3";
    TestCommandInvocationHandle iHandle("ls", {"dir3"});
    ;
    CommandResult result = ls.invoke(iHandle.invocation);

    // The order is not garuanteed
    TEST_ASSERT_TRUE(iHandle.output.contains("foo.txt\t"));
    TEST_ASSERT_TRUE(iHandle.output.contains("bar.txt\t"));
    TEST_ASSERT_TRUE(iHandle.output.contains("baz.txt\t"));
}

void test_ls_flag_l(void)
{
    cmd::ls ls(*dir);
    ETString keyword = "ls";
    ETString arg = "-l dir2";
    TestCommandInvocationHandle iHandle("ls", {"-l", "dir2"});
    ;
    CommandResult result = ls.invoke(iHandle.invocation);
    // Should show foo.txt with details (simulate long listing)
    TEST_ASSERT_TRUE(iHandle.output.contains("foo.txt"));
    TEST_ASSERT_FALSE(iHandle.output.contains("bar")); // Content should not be shown
    // Check for typical long listing info (e.g., size, type)
    TEST_ASSERT_TRUE(iHandle.output.contains("Bytes"));
}

void test_ls_nonexistent_directory(void)
{
    cmd::ls ls(*dir);
    ETString keyword = "ls";
    ETString arg = "non_exist";
    TestCommandInvocationHandle iHandle("ls", {"non_exist"});
    ;
    CommandResult result = ls.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains("is not a directory"));
}

void test_ls_execute_writes_stdout(void)
{
    cmd::ls ls(*dir);

    TestCommandInvocationHandle iHandle("ls", {"dir2"});
    ;
    CommandResult result = ls.invoke(iHandle.invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(iHandle.output.contains("foo.txt"));
    TEST_ASSERT_FALSE(iHandle.error.contains("bar.txt"));
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_ls_valid_directory);
    RUN_TEST(test_ls_nonexistent_directory);
    RUN_TEST(test_ls_edge_cases);
    RUN_TEST(test_ls_output_format);
    RUN_TEST(test_ls_flag_l);
    RUN_TEST(test_ls_execute_writes_stdout);
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
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
#include "../Mocks/MockFileSystem.h"
#include "../Mocks/MockStream.h"
#include "../Mocks/CommandRuntimeTestUtils.h"
#include "DirectoryNavigator.h"
#include "StorageSystem.h"
#include "../Mocks/MockStorageMedia.h"

IStorageSystem *storage = nullptr;
DirectoryNavigator *dir = nullptr;

void setUp(void)
{
    storage = new StorageSystem();
    auto media = new MockStorageMedia("mock", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
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
    auto medias = storage->media();
    for (auto media : medias)
    {
        storage->unmountMedia(media->name());
        delete media;
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
    ETString arg = "-l dir2";
    ETString result = ls.trigger(keyword, arg);
    // Should show foo.txt with details (simulate long listing)
    TEST_ASSERT_TRUE(result.find("foo.txt") != ETString::npos);
    TEST_ASSERT_FALSE(result.find("bar") != ETString::npos); // Content should not be shown
    // Check for typical long listing info (e.g., size, type)
    TEST_ASSERT_TRUE(result.find("Bytes") != ETString::npos);
}

void test_ls_edge_cases(void)
{
    cmd::ls ls(*dir);
    ETString keyword = "ls";
    ETString arg = "   ";
    ETString result = ls.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("file.txt") != ETString::npos);
}

void test_ls_output_format(void)
{
    cmd::ls ls(*dir);
    ETString keyword = "ls";
    ETString arg = "dir3";
    ETString result = ls.trigger(keyword, arg);

    // The order is not garuanteed
    TEST_ASSERT_TRUE(result.find("foo.txt\t") != ETString::npos);
    TEST_ASSERT_TRUE(result.find("bar.txt\t") != ETString::npos);
    TEST_ASSERT_TRUE(result.find("baz.txt\t") != ETString::npos);
}

void test_ls_flag_l(void)
{
    cmd::ls ls(*dir);
    ETString keyword = "ls";
    ETString arg = "-l dir2";
    ETString result = ls.trigger(keyword, arg);
    // Should show foo.txt with details (simulate long listing)
    TEST_ASSERT_TRUE(result.find("foo.txt") != ETString::npos);
    TEST_ASSERT_TRUE(result.find("bar") == ETString::npos); // Content should not be shown
    // Check for typical long listing info (e.g., size, type)
    TEST_ASSERT_TRUE(result.find("Bytes") != ETString::npos);
}

void test_ls_nonexistent_directory(void)
{
    cmd::ls ls(*dir);
    ETString keyword = "ls";
    ETString arg = "non_exist";
    ETString result = ls.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("is not a directory") != ETString::npos);
}

void test_ls_execute_writes_stdout(void)
{
    cmd::ls ls(*dir);

    MockStream stream;
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{"ls", "dir3", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = ls.execute(invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("foo.txt") != ETString::npos);
    TEST_ASSERT_TRUE(stream.stderrBuffer.empty());
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
// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "commands/rmdir.h"
#include "../../Mocks/native/MockFileSystem.h"
#include "../../Mocks/MockStream.h"
#include "../../Mocks/CommandRuntimeTestUtils.h"
#include "DirectoryNavigator.h"

#include "StorageSystem.h"
#include "../../Mocks/native/MockStorageMedia.h"

IStorageSystem *storage = nullptr;
DirectoryNavigator *dir = nullptr;

void setUp(void)
{
    storage = new StorageSystem();
    auto media = new MockStorageMedia("mock", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storage->mountMedia(media, "/");
    dir = new DirectoryNavigator(storage);
    storage->mkdir("dir1");
    storage->open("file.txt", "w", true).writeAll("hello");
    storage->mkdir("foo/bar/baz");
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

void test_rmdir_valid_directory(void)
{
    cmd::rmdir rmdir(*dir);
    ETString keyword = "rmdir";
    ETString arg = "dir1";
    ETString result = rmdir.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("removed") != ETString::npos);
}

void test_rmdir_nonexistent_directory(void)
{
    cmd::rmdir rmdir(*dir);
    ETString keyword = "rmdir";
    ETString arg = "no_dir";
    ETString result = rmdir.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("did not exist") != ETString::npos);
}

void test_rmdir_file_instead_of_directory(void)
{
    cmd::rmdir rmdir(*dir);
    ETString keyword = "rmdir";
    ETString arg = "file.txt";
    ETString result = rmdir.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("not a directory") != ETString::npos);
}

void test_rmdir_usage(void)
{
    cmd::rmdir rmdir(*dir);
    ETString keyword = "rmdir";
    ETString result = rmdir.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Remove the specfied folder") != ETString::npos);
}

void test_rmdir_edge_cases(void)
{
    cmd::rmdir rmdir(*dir);
    ETString keyword = "rmdir";
    ETString arg = "   ";
    ETString result = rmdir.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("Can not remove folder with no name\n") != ETString::npos);
}

void test_rmdir_subdirectory(void)
{
    cmd::rmdir rmdir(*dir);
    ETString keyword = "rmdir";
    ETString arg = "/foo/bar/baz";
    ETString result = rmdir.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("removed") != ETString::npos);
    TEST_ASSERT_TRUE(storage->exists("/foo/bar"));
    TEST_ASSERT_FALSE(storage->exists("/foo/bar/baz"));
}

void test_rmdir_notemptydirectory(void)
{
    cmd::rmdir rmdir(*dir);
    ETString keyword = "rmdir";
    ETString arg = "/foo/bar";
    ETString result = rmdir.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("is not empty") != ETString::npos);
    TEST_ASSERT_TRUE(storage->exists("/foo/bar/baz"));
    TEST_ASSERT_TRUE(storage->exists("/foo/bar"));
}

void test_rmdir_auto_completion_directory_suggestions(void)
{
    cmd::rmdir rmdir(*dir);
    ETVector<ETString> suggestions = rmdir.getSuggestions("di");
    TEST_ASSERT_TRUE(suggestions.size() > 0);
    TEST_ASSERT_TRUE(suggestions[0].contains("dir"));
}

void test_rmdir_auto_completion_no_files(void)
{
    cmd::rmdir rmdir(*dir);
    ETVector<ETString> suggestions = rmdir.getSuggestions("fi");
    // Should not suggest files, only directories
    TEST_ASSERT_TRUE(suggestions.size() == 0 || !suggestions[0].contains(".txt"));
}

void test_rmdir_execute_writes_stdout(void)
{
    cmd::rmdir rmdir(*dir);

    MockStream stream;
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{"rmdir", "dir1", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = rmdir.execute(invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("removed") != ETString::npos);
    TEST_ASSERT_TRUE(stream.stderrBuffer.empty());
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_rmdir_valid_directory);
    RUN_TEST(test_rmdir_nonexistent_directory);
    RUN_TEST(test_rmdir_file_instead_of_directory);
    RUN_TEST(test_rmdir_usage);
    RUN_TEST(test_rmdir_edge_cases);
    RUN_TEST(test_rmdir_subdirectory);
    RUN_TEST(test_rmdir_notemptydirectory);
    RUN_TEST(test_rmdir_auto_completion_directory_suggestions);
    RUN_TEST(test_rmdir_auto_completion_no_files);
    RUN_TEST(test_rmdir_execute_writes_stdout);
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

// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "commands/rm.h"
#include "../../Mocks/native/MockFileSystem.h"
#include "../utils.h"
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
    storage->open("file.txt", "w", true).writeAll("hello"); // File
    storage->mkdir("dir1");                                 // Directory
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

using namespace EmbeddedTerminal;

void test_rm_valid_file(void)
{
    cmd::rm rm(*dir);
    ETString keyword = "rm";
    ETString arg = "file.txt";
    auto iHandle = TestCommandInvocationHandle("rm", {arg});
    CommandResult result = rm.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains("removed"));
}

void test_rm_nonexistent_file(void)
{
    cmd::rm rm(*dir);
    ETString keyword = "rm";
    ETString arg = "no_file.txt";
    auto iHandle = TestCommandInvocationHandle("rm", {arg});
    CommandResult result = rm.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains("did not exist"));
}

void test_rm_directory_instead_of_file(void)
{
    cmd::rm rm(*dir);
    ETString keyword = "rm";
    ETString arg = "dir1";
    auto iHandle = TestCommandInvocationHandle("rm", {arg});
    CommandResult result = rm.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains("not a file"));
}

void test_rm_usage(void)
{
    cmd::rm rm(*dir);
    ETString keyword = "rm";
    ETString result = rm.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Remove the specified file\n") != ETString::npos);
}

void test_rm_edge_cases(void)
{
    cmd::rm rm(*dir);
    ETString keyword = "rm";
    auto iHandle = TestCommandInvocationHandle("rm");
    CommandResult result = rm.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains(rm.usage(keyword)));
}

void test_rm_auto_completion_file_suggestions(void)
{
    cmd::rm rm(*dir);
    ETVector<ETString> suggestions = rm.getSuggestions("fi");
    TEST_ASSERT_TRUE(suggestions.size() > 0);
    TEST_ASSERT_TRUE(suggestions[0].contains("file"));
}

void test_rm_auto_completion_directory_suggestions(void)
{
    cmd::rm rm(*dir);
    ETVector<ETString> suggestions = rm.getSuggestions("di");
    TEST_ASSERT_TRUE(suggestions.size() > 0);
    TEST_ASSERT_TRUE(suggestions[0].contains("dir"));
}

void test_rm_execute_writes_stdout(void)
{
    cmd::rm rm(*dir);

    auto iHandle = TestCommandInvocationHandle("rm", {"file.txt"});
    CommandResult result = rm.invoke(iHandle.invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(iHandle.output.contains("removed"));
    TEST_ASSERT_TRUE(iHandle.error.empty());
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_rm_valid_file);
    RUN_TEST(test_rm_nonexistent_file);
    RUN_TEST(test_rm_directory_instead_of_file);
    RUN_TEST(test_rm_usage);
    RUN_TEST(test_rm_edge_cases);
    RUN_TEST(test_rm_auto_completion_file_suggestions);
    RUN_TEST(test_rm_auto_completion_directory_suggestions);
    RUN_TEST(test_rm_execute_writes_stdout);
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
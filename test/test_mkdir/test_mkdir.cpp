// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "commands/mkdir.h"
#include "../Mocks/native/MockFileSystem.h"
#include "../Mocks/MockStream.h"
#include "../Mocks/CommandRuntimeTestUtils.h"
#include "DirectoryNavigator.h"
#include "StorageSystem.h"
#include "../Mocks/native/MockStorageMedia.h"

IStorageSystem *storage = nullptr;
DirectoryNavigator *dir = nullptr;

void setUp(void)
{
    storage = new StorageSystem();
    auto media = new MockStorageMedia("mock", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storage->mountMedia(media, "/");
    dir = new DirectoryNavigator(storage);
    storage->mkdir("existingdir");
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

void test_mkdir_valid_directory(void)
{
    cmd::mkdir mkdir(*dir);
    ETString keyword = "mkdir";
    ETString arg = "newdir";
    ETString result = mkdir.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("created") != ETString::npos);
}

void test_mkdir_existing_directory(void)
{
    cmd::mkdir mkdir(*dir);
    ETString keyword = "mkdir";
    ETString arg = "existingdir";
    ETString result = mkdir.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("already exists") != ETString::npos);
}

void test_mkdir_usage(void)
{
    cmd::mkdir mkdir(*dir);
    ETString keyword = "mkdir";
    ETString result = mkdir.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Create the specified folder") != ETString::npos);
}

void test_mkdir_edge_cases(void)
{
    cmd::mkdir mkdir(*dir);
    ETString keyword = "mkdir";
    ETString arg = "   ";
    ETString result = mkdir.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("Can not create folder with no name\n") != ETString::npos);
}

void test_mkdir_auto_completion_directory_suggestions(void)
{
    cmd::mkdir mkdir(*dir);
    ETVector<ETString> suggestions = mkdir.getSuggestions("exi");
    // Should suggest existing directories
    TEST_ASSERT_TRUE(suggestions.size() > 0);
}

void test_mkdir_execute_writes_stdout(void)
{
    cmd::mkdir mkdir(*dir);

    MockStream stream;
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{"mkdir", "newdir2", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = mkdir.execute(invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("created") != ETString::npos);
    TEST_ASSERT_TRUE(stream.stderrBuffer.empty());
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_mkdir_valid_directory);
    RUN_TEST(test_mkdir_existing_directory);
    RUN_TEST(test_mkdir_usage);
    RUN_TEST(test_mkdir_edge_cases);
    RUN_TEST(test_mkdir_auto_completion_directory_suggestions);
    RUN_TEST(test_mkdir_execute_writes_stdout);
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
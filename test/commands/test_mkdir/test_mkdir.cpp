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
    storage->mkdir("existingdir");
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

void test_mkdir_valid_directory(void)
{
    cmd::mkdir mkdir(*dir);
    TestCommandInvocationHandle iHandle("mkdir", {"newdir"});
    ;
    CommandResult result = mkdir.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains("created"));
}

void test_mkdir_existing_directory(void)
{
    cmd::mkdir mkdir(*dir);
    TestCommandInvocationHandle iHandle("mkdir", {"existingdir"});
    ;
    CommandResult result = mkdir.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains("already exists"));
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
    TestCommandInvocationHandle iHandle("mkdir");
    ;
    CommandResult result = mkdir.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains(mkdir.usage(keyword)));
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

    TestCommandInvocationHandle iHandle("mkdir", {"newdir"});
    ;
    CommandResult result = mkdir.invoke(iHandle.invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_MESSAGE(("Output: " + iHandle.output).c_str());
    TEST_ASSERT_TRUE(iHandle.output.contains("created"));
    TEST_ASSERT_TRUE(iHandle.error.empty());
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
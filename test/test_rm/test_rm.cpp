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
#include <string>
#include "../Mocks/MockFileSystem.h"

void setUp(void) {}
void tearDown(void) {}

using namespace EmbeddedTerminal;
MockFileSystem FS;
DirectoryNavigator dir(&FS);
class TestRm : public cmd::rm
{

public:
    TestRm() : cmd::rm(dir)
    {
        // Setup mock files
        FS.createFile("file.txt", "hello", 5); // File
        FS.createDirectory("dir1");            // Directory
    }
};

void test_rm_valid_file(void)
{
    TestRm rm;
    ETString keyword = "rm";
    ETString arg = "file.txt";
    ETString result = rm.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("removed") != ETString::npos);
}

void test_rm_nonexistent_file(void)
{
    TestRm rm;
    ETString keyword = "rm";
    ETString arg = "no_file.txt";
    ETString result = rm.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("did not exist") != ETString::npos);
}

void test_rm_directory_instead_of_file(void)
{
    TestRm rm;
    ETString keyword = "rm";
    ETString arg = "dir1";
    ETString result = rm.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("not a file") != ETString::npos);
}

void test_rm_usage(void)
{
    TestRm rm;
    ETString keyword = "rm";
    ETString result = rm.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Remove the specified file\n") != ETString::npos);
}

void test_rm_edge_cases(void)
{
    TestRm rm;
    ETString keyword = "rm";
    ETString arg = "   ";
    ETString result = rm.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("Can not remove unspecified file!\n") != ETString::npos);
}

void test_rm_auto_completion_file_suggestions(void)
{
    TestRm rm;
    ETVector<ETString> suggestions = rm.getSuggestions("fi");
    TEST_ASSERT_TRUE(suggestions.size() > 0);
    TEST_ASSERT_TRUE(suggestions[0].contains("file"));
}

void test_rm_auto_completion_directory_suggestions(void)
{
    TestRm rm;
    ETVector<ETString> suggestions = rm.getSuggestions("di");
    TEST_ASSERT_TRUE(suggestions.size() > 0);
    TEST_ASSERT_TRUE(suggestions[0].contains("dir"));
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
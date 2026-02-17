// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "commands/cat.h"
#include "../src/DirectoryNavigator.h"
#include "../Mocks/MockFileSystem.h"

#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

void test_cat_trigger_small_file(void)
{
    MockFileSystem fs;
    EmbeddedTerminal::DirectoryNavigator dir(&fs);
    fs.createFile("/file.txt", "hello1234", 9); // Small file
    EmbeddedTerminal::cmd::cat cat(dir);

    ETString keyword = "cat";
    ETString arg = "file.txt";
    ETString result = cat.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("hello1234") != ETString::npos);
}

void test_cat_trigger_large_file(void)
{
    MockFileSystem fs;
    EmbeddedTerminal::DirectoryNavigator dir(&fs);
    fs.createFile("/big.txt", "ABC", 2000); // Fake large file
    EmbeddedTerminal::cmd::cat cat(dir);
    ETString keyword = "cat";
    ETString arg = "big.txt";
    ETString result = cat.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("... File truncated ...") != ETString::npos);
}

void test_cat_trigger_file_not_exists(void)
{
    MockFileSystem fs;
    EmbeddedTerminal::DirectoryNavigator dir(&fs);
    EmbeddedTerminal::cmd::cat cat(dir);

    ETString keyword = "cat";
    ETString arg = "nofile.txt";
    ETString result = cat.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("did not exist!") != ETString::npos);
}

void test_cat_usage(void)
{
    MockFileSystem fs;
    EmbeddedTerminal::DirectoryNavigator dir(&fs);
    EmbeddedTerminal::cmd::cat cat(dir);
    ETString keyword = "cat";
    ETString result = cat.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Returns the content") != ETString::npos);
}

void test_cat_trigger_edge_cases(void)
{
    MockFileSystem fs;
    EmbeddedTerminal::DirectoryNavigator dir(&fs);
    EmbeddedTerminal::cmd::cat cat(dir);
    ETString keyword = "cat";
    ETString arg = "   ";
    ETString result = cat.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("expected") != ETString::npos);
    ETString arg2 = "did_not_exist.txt";
    ETString result2 = cat.trigger(keyword, arg2);
    TEST_ASSERT_TRUE(result2.find("did not exist!") != ETString::npos);
}

int process_tests_cat()
{
    UNITY_BEGIN();
    RUN_TEST(test_cat_trigger_small_file);
    RUN_TEST(test_cat_trigger_large_file);
    RUN_TEST(test_cat_trigger_file_not_exists);
    RUN_TEST(test_cat_usage);
    RUN_TEST(test_cat_trigger_edge_cases);
    UNITY_END();
    return 0;
}

#if (defined(ESP_PLATFORM) || defined(ESP32)) && not defined(ARDUINO)
extern "C" void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(4000));
    process_tests_cat();
}
#elif defined(ARDUINO)
void setup()
{
    delay(2500);
    process_tests_cat();
}
void loop() {}
#else
int main()
{
    return process_tests_cat();
}
#endif

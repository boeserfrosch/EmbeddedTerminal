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
#include <string>
#include "../Mocks/MockFileSystem.h"
#include "DirectoryNavigator.h"

void setUp(void) {}
void tearDown(void) {}

MockFileSystem FS;
DirectoryNavigator dir(&FS);

class TestLs : public cmd::ls
{

public:
    TestLs() : cmd::ls(dir)
    {
        // Setup mock files and directories
        FS.createFile("/file.txt", "hello", 5);
        FS.createDirectory("/dir1");
        FS.createDirectory("/dir2");
        FS.createFile("/dir2/foo.txt", "bar", 4);
        FS.createFile("/dir3/foo.txt", "bar", 4);
        FS.createFile("/dir3/bar.txt", "bar", 4);
        FS.createFile("/dir3/baz.txt", "bar", 4);
    }
};

void test_ls_valid_directory(void)
{
    TestLs ls;
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
    TestLs ls;
    ETString keyword = "ls";
    ETString arg = "   ";
    ETString result = ls.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("file.txt") != ETString::npos);
}

void test_ls_output_format(void)
{
    TestLs ls;
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
    TestLs ls;
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
    TestLs ls;
    ETString keyword = "ls";
    ETString arg = "non_exist";
    ETString result = ls.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("is not a directory") != ETString::npos);
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_ls_valid_directory);
    RUN_TEST(test_ls_nonexistent_directory);
    RUN_TEST(test_ls_edge_cases);
    RUN_TEST(test_ls_output_format);
    RUN_TEST(test_ls_flag_l);
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
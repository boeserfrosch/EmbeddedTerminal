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
#include <string>
#include "../Mocks/MockFileSystem.h"
#include "DirectoryNavigator.h"

void setUp(void) {}
void tearDown(void) {}
MockFileSystem FS;
DirectoryNavigator dir(&FS);

class TestMkdir : public cmd::mkdir
{

public:
    TestMkdir() : cmd::mkdir(dir)
    {
        // Setup mock directories
        FS.createDirectory("existingdir");
    }
};

void test_mkdir_valid_directory(void)
{
    TestMkdir mkdir;
    ETString keyword = "mkdir";
    ETString arg = "newdir";
    ETString result = mkdir.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("created") != ETString::npos);
}

void test_mkdir_existing_directory(void)
{
    TestMkdir mkdir;
    ETString keyword = "mkdir";
    ETString arg = "existingdir";
    ETString result = mkdir.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("already exists") != ETString::npos);
}

void test_mkdir_usage(void)
{
    TestMkdir mkdir;
    ETString keyword = "mkdir";
    ETString result = mkdir.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Create the specified folder") != ETString::npos);
}

void test_mkdir_edge_cases(void)
{
    TestMkdir mkdir;
    ETString keyword = "mkdir";
    ETString arg = "   ";
    ETString result = mkdir.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("Can not create folder with no name\n") != ETString::npos);
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_mkdir_valid_directory);
    RUN_TEST(test_mkdir_existing_directory);
    RUN_TEST(test_mkdir_usage);
    RUN_TEST(test_mkdir_edge_cases);
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
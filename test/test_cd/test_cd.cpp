// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "../src/commands/cd.h"
#include "../Mocks/MockFileSystem.h"

void setUp(void) {}
void tearDown(void) {}

void test_cd_trigger_valid_path(void)
{
    MockFileSystem fs;
    fs.createDirectory("/valid");
    DirectoryNavigator dir(&fs);
    cmd::cd cd(dir);
    ETString keyword = "cd";
    ETString additional = "/valid";
    ETString result = cd.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("> valid") != ETString::npos || result.find("\n") != ETString::npos);
}

void test_cd_trigger_invalid_path(void)
{
    MockFileSystem fs;
    DirectoryNavigator dir(&fs);
    cmd::cd cd(dir);
    ETString keyword = "cd";
    ETString additional = "/invalid";
    ETString result = cd.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("did not exist") != ETString::npos);

    // Test with a file path that is not a directory
    fs.createFile("not_a_dir.txt", "content", 7);
    ETString filePath = "not_a_dir.txt";
    ETString result2 = cd.trigger(keyword, filePath);
    TEST_ASSERT_TRUE(result2.find("not a directory") != ETString::npos);
}

void test_cd_usage(void)
{
    MockFileSystem fs;
    DirectoryNavigator dir(&fs);
    cmd::cd cd(dir);
    ETString keyword = "cd";
    ETString result = cd.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Change the current directory") != ETString::npos);
}

void test_cd_empty_keyword(void)
{
    MockFileSystem fs;
    DirectoryNavigator dir(&fs);
    cmd::cd cd(dir);
    ETString keyword = "";
    ETString additional = "";
    ETString result = cd.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("Expected parameter") != ETString::npos);
}

void test_cd_pwd_cd_back_to_pwd(void)
{
    MockFileSystem FS;
    FS.createDirectory("/");
    FS.createDirectory("/folder");
    FS.createDirectory("/folder/another");
    FS.createDirectory("/folder/another/deeper");
    FS.createDirectory("/folder2");
    DirectoryNavigator dir(&FS);
}

int process_tests_cd()
{

#ifndef COMBINED_TESTS
    UNITY_BEGIN();
#endif
    RUN_TEST(test_cd_trigger_valid_path);
    RUN_TEST(test_cd_trigger_invalid_path);
    RUN_TEST(test_cd_usage);
    RUN_TEST(test_cd_empty_keyword);
    RUN_TEST(test_cd_pwd_cd_back_to_pwd);
#ifndef COMBINED_TESTS
    UNITY_END();
#endif
    return 0;
}
#if (defined(ESP_PLATFORM) || defined(ESP32)) && not defined(ARDUINO)
extern "C" void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(4000));
    process_tests_cd();
}
#elif defined(ARDUINO)
void setup()
{
    delay(2500);
    process_tests_cd();
}
void loop() {}
#else
int main()
{
    return process_tests_cd();
}
#endif

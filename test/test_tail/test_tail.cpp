// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "commands/tail.h"
#include <string>
#include "../Mocks/MockFileSystem.h"
#include "DirectoryNavigator.h"
#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

using namespace EmbeddedTerminal;
MockFileSystem FS;
DirectoryNavigator dir(&FS);
class TestTail : public cmd::tail
{

public:
    TestTail() : cmd::tail(dir)
    {
        FS.createFile("file.txt", "hello1234", 9);
        FS.createFile("big.txt", "ABC", 2000);
    }
};

void test_tail_trigger_small_file(void)
{
    TestTail tail;
    ETString keyword = "tail";
    ETString arg = "file.txt";
    ETString result = tail.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("hello1234") != ETString::npos);
}

void test_tail_trigger_large_file(void)
{
    TestTail tail;
    ETString keyword = "tail";
    ETString arg = "big.txt";
    ETString result = tail.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("... File truncated ...") != ETString::npos);
}

void test_tail_trigger_file_not_exists(void)
{
    TestTail tail;
    ETString keyword = "tail";
    ETString arg = "nofile.txt";
    ETString result = tail.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("did not exist!") != ETString::npos);
}

void test_tail_usage(void)
{
    TestTail tail;
    ETString keyword = "tail";
    ETString result = tail.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Returns the last") != ETString::npos);
}

void test_tail_trigger_edge_cases(void)
{
    TestTail tail;
    ETString keyword = "tail";
    ETString arg = "   ";
    ETString result = tail.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("Expected parameter") != ETString::npos);
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_tail_trigger_small_file);
    RUN_TEST(test_tail_trigger_large_file);
    RUN_TEST(test_tail_trigger_file_not_exists);
    RUN_TEST(test_tail_usage);
    RUN_TEST(test_tail_trigger_edge_cases);
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
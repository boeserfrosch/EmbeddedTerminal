#include "../Mocks/MockStream.h"
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif

void test_available_and_readAll()
{
    MockStream stream;
    stream.inputBuffer = "hello\nworld\n";
    TEST_ASSERT_TRUE(stream.available());
    ETString line1 = stream.readAll();
    TEST_ASSERT_EQUAL_STRING(line1.c_str(), "hello\nworld\n");
    TEST_ASSERT_FALSE(stream.available());
}

void test_print_and_printf()
{
    MockStream stream;
    stream.print("abc\n");
    TEST_ASSERT_EQUAL_STRING(stream.outputBuffer.c_str(), "abc\n");
    stream.printf("%d-%s", 42, "test");
    TEST_ASSERT_TRUE(stream.outputBuffer.find("42-test") != ETString::npos);
}

void setUp(void) {}
void tearDown(void) {}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_available_and_readAll);
    RUN_TEST(test_print_and_printf);
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

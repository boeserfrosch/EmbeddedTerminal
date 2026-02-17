#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "../Mocks/MockCommand.h"
#include "../src/ETTypes.h"

using namespace EmbeddedTerminal;

class TestableMockCommand : public MockCommand
{
public:
    ETString test_trigger(ETString &keyword, ETString &additional)
    {
        return trigger(keyword, additional);
    }
};

void setUp(void) {}
void tearDown(void) {}

void test_mockcommand_usage()
{
    TestableMockCommand cmd;
    ETString keyword = "test";
    ETString usage = cmd.usage(keyword);
    TEST_ASSERT_TRUE(usage.find("Usage: test") != ETString::npos);
}

void test_mockcommand_trigger()
{
    TestableMockCommand cmd;
    ETString keyword = "foo";
    ETString additional = "bar";
    ETString result = cmd.test_trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("Triggered: foo bar") != ETString::npos);
    TEST_ASSERT_EQUAL_STRING("foo", cmd.lastKeyword.c_str());
    TEST_ASSERT_EQUAL_STRING("bar", cmd.lastAdditional.c_str());
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_mockcommand_usage);
    RUN_TEST(test_mockcommand_trigger);
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
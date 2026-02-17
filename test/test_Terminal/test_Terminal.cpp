// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#endif

#include "../src/Terminal.h"
#include "../src/ETTypes.h"
#include <string>
#include <map>
#include "../Mocks/MockStream.h"
#include "../Mocks/MockCommand.h"
#include <unity.h>

using namespace EmbeddedTerminal;

void setUp(void) {}
void tearDown(void) {}

void test_terminal_register_and_call(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("test", &cmd);
    stream.inputBuffer = "test extra\n\r";
    term.loop();

    TEST_ASSERT_TRUE(stream.outputBuffer.find("test extra") != ETString::npos);
    TEST_ASSERT_TRUE(stream.outputBuffer.find("Triggered: test extra") != ETString::npos);
    TEST_ASSERT_EQUAL_STRING("test", cmd.lastKeyword.c_str());
    TEST_ASSERT_EQUAL_STRING("extra", cmd.lastAdditional.c_str());
}

void test_terminal_call_unknown(void)
{
    MockStream stream;
    Terminal term(stream);
    stream.inputBuffer = "foo\n";
    term.loop();
    TEST_ASSERT_TRUE(stream.outputBuffer.find("foo is unknown") != ETString::npos);
}

void test_terminal_getCommands(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("foo", &cmd);
    auto cmds = term.getCommands();
    TEST_ASSERT_TRUE(cmds.find("foo") != cmds.end());
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_terminal_register_and_call);
    RUN_TEST(test_terminal_call_unknown);
    RUN_TEST(test_terminal_getCommands);
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
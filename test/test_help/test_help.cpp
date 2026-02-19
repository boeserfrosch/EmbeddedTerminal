// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "commands/help.h"
#include "../Mocks/MockStream.h"
#include <string>

using namespace EmbeddedTerminal;

void setUp(void) {}
void tearDown(void) {}

class DummyCommand : public ICommand
{
public:
    ETString trigger(const ETString &keyword, const ETString &additional) override
    {
        return "Dummy command triggered";
    }
    ETString usage(const ETString &keyword) override
    {
        return "Dummy usage";
    }
};

MockStream stream;
Terminal terminal(stream);
class TestHelp : public cmd::help
{

public:
    TestHelp() : cmd::help(terminal)
    {
        terminal.registerCommand("dummy", new DummyCommand());
    }
};

void test_help_valid_command(void)
{
    TestHelp help;
    ETString keyword = "help";
    ETString arg = "dummy";
    ETString result = help.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("Dummy usage") != ETString::npos);
}

void test_help_invalid_command(void)
{
    TestHelp help;
    ETString keyword = "help";
    ETString arg = "unknown";
    ETString result = help.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("Unknown command!") != ETString::npos);
}

void test_help_usage(void)
{
    TestHelp help;
    ETString keyword = "help";
    ETString result = help.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Returns all available commands") != ETString::npos);
}

void test_help_edge_cases(void)
{
    TestHelp help;
    ETString keyword = "help";
    ETString arg = "   ";
    ETString result = help.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("Available commands") != ETString::npos);
}

void test_help_output_format(void)
{
    TestHelp help;
    ETString keyword = "help";
    ETString arg = "dummy";
    ETString result = help.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("Dummy usage") != ETString::npos);
}

void test_help_auto_completion_command_suggestions(void)
{
    TestHelp help;
    ETVector<ETString> suggestions = help.getSuggestions("du");
    TEST_ASSERT_TRUE(suggestions.size() > 0);
    TEST_ASSERT_TRUE(suggestions[0].contains("dummy"));
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_help_valid_command);
    RUN_TEST(test_help_invalid_command);
    RUN_TEST(test_help_usage);
    RUN_TEST(test_help_edge_cases);
    RUN_TEST(test_help_output_format);
    RUN_TEST(test_help_auto_completion_command_suggestions);
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

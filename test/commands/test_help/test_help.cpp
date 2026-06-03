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
#include "Terminal.h"
#include "../utils.h"

using namespace EmbeddedTerminal;

void setUp(void) {}
void tearDown(void) {}

MockStream stream;
Terminal terminal(stream);

class DummyCommand : public ICommand
{
public:
    ETString usage(const ETString &keyword) const override
    {
        return "Dummy usage: dummy [options]";
    }

    CommandResult invoke(CommandInvocation &invocation) override
    {
        invocation.streams.output.print("Dummy command executed with arguments: ");
        for (const auto &arg : invocation.arguments)
        {
            invocation.streams.output.print(arg + " ");
        }
        invocation.streams.output.print("\n");
        return CommandResult::completed(0);
    }
};

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
    TestCommandInvocationHandle iHandle(keyword, {arg});
    ;
    CommandResult result = help.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains("Dummy usage"));
}

void test_help_invalid_command(void)
{
    TestHelp help;
    ETString keyword = "help";
    ETString arg = "unknown";

    TestCommandInvocationHandle iHandle(keyword, {arg});
    ;
    CommandResult result = help.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE_MESSAGE(iHandle.output.contains("Unknown command: 'unknown'"), iHandle.output.c_str());
}

void test_help_usage(void)
{
    TestHelp help;
    ETString keyword = "help";
    ETString result = help.usage(keyword);
    TEST_ASSERT_TRUE_MESSAGE(result.find("Returns all available commands") != ETString::npos, result.c_str());
}

void test_help_edge_cases(void)
{
    TestHelp help;
    ETString keyword = "help";
    TestCommandInvocationHandle iHandle(keyword, {});
    ;
    CommandResult result = help.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE_MESSAGE(iHandle.output.contains("Available commands"), iHandle.output.c_str());
}

void test_help_output_format(void)
{
    TestHelp help;
    ETString keyword = "help";
    ETString arg = "dummy";
    TestCommandInvocationHandle iHandle(keyword, {arg});
    ;
    CommandResult result = help.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE_MESSAGE(iHandle.output.contains("Dummy usage"), iHandle.output.c_str());
}

void test_help_auto_completion_command_suggestions(void)
{
    TestHelp help;
    ETVector<ETString> suggestions = help.getSuggestions("du");
    TEST_ASSERT_TRUE(suggestions.size() > 0);
    TEST_ASSERT_TRUE(suggestions[0].contains("dummy"));
}

void test_help_execute_writes_stdout(void)
{
    TestHelp help;

    TestCommandInvocationHandle iHandle("help", {"dummy"});
    ;
    CommandResult result = help.invoke(iHandle.invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(iHandle.output.contains("Dummy usage"));
    TEST_ASSERT_FALSE(iHandle.error.contains("Dummy usage"));
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
    RUN_TEST(test_help_execute_writes_stdout);
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

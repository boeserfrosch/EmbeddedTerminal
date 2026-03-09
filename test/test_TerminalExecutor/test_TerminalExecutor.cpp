// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#endif

#include <unity.h>
#include "TerminalExecutor.h"

using namespace EmbeddedTerminal;

void setUp(void) {}
void tearDown(void) {}

void test_executor_runs_simple_chain(void)
{
    ETVector<ParsedCommand> executed;
    int exitCode = 0;

    TerminalExecutor executor(
        [&executed](const ParsedCommand &command)
        {
            executed.push_back(command);
        },
        [&exitCode]() { return exitCode; },
        [](const ETString &input, const ETString &, const ETString &) { return input; });

    ParsedAst ast;
    ast.isForLoop = false;
    ParsedChainSegment segment;
    segment.command.keywords.push_back("echo");
    segment.command.arguments.push_back("hi");
    ast.chain.segments.push_back(segment);

    executor.execute(ast);

    TEST_ASSERT_EQUAL(1, executed.size());
    TEST_ASSERT_EQUAL_STRING("echo", executed[0].keywords[0].c_str());
    TEST_ASSERT_EQUAL_STRING("hi", executed[0].arguments[0].c_str());
}

void test_executor_respects_andand(void)
{
    ETVector<ParsedCommand> executed;
    int exitCode = 1;

    TerminalExecutor executor(
        [&executed, &exitCode](const ParsedCommand &command)
        {
            executed.push_back(command);
            exitCode = (command.keywords[0] == "ok") ? 0 : 1;
        },
        [&exitCode]() { return exitCode; },
        [](const ETString &input, const ETString &, const ETString &) { return input; });

    ParsedAst ast;
    ParsedChainSegment first;
    first.condition = ChainCondition::Always;
    first.command.keywords.push_back("ok");
    first.command.arguments.push_back("");

    ParsedChainSegment second;
    second.condition = ChainCondition::OnSuccess;
    second.command.keywords.push_back("next");
    second.command.arguments.push_back("");

    ast.chain.segments.push_back(first);
    ast.chain.segments.push_back(second);

    executor.execute(ast);

    TEST_ASSERT_EQUAL(2, executed.size());
    TEST_ASSERT_EQUAL_STRING("next", executed[1].keywords[0].c_str());
}

void test_executor_respects_oror(void)
{
    ETVector<ParsedCommand> executed;
    int exitCode = 1;

    TerminalExecutor executor(
        [&executed, &exitCode](const ParsedCommand &command)
        {
            executed.push_back(command);
            exitCode = 1;
        },
        [&exitCode]() { return exitCode; },
        [](const ETString &input, const ETString &, const ETString &) { return input; });

    ParsedAst ast;
    ParsedChainSegment seg;
    seg.condition = ChainCondition::OnFailure;
    seg.command.keywords.push_back("recover");
    seg.command.arguments.push_back("");
    ast.chain.segments.push_back(seg);

    executor.execute(ast);

    TEST_ASSERT_EQUAL(1, executed.size());
    TEST_ASSERT_EQUAL_STRING("recover", executed[0].keywords[0].c_str());
}

void test_executor_expands_for_loop_values(void)
{
    ETVector<ParsedCommand> executed;
    int exitCode = 0;

    TerminalExecutor executor(
        [&executed](const ParsedCommand &command)
        {
            executed.push_back(command);
        },
        [&exitCode]() { return exitCode; },
        [](const ETString &input, const ETString &name, const ETString &value)
        {
            ETString result = input;
            ETString pattern = "$" + name;
            size_t pos = result.find(pattern);
            if (pos != ETString::npos)
            {
                ETString left = result.substr(0, pos);
                ETString right = result.substr(pos + pattern.length());
                result = left + value + right;
            }
            return result;
        });

    ParsedAst ast;
    ast.isForLoop = true;
    ast.forLoop.variable = "i";
    ast.forLoop.values.push_back("a");
    ast.forLoop.values.push_back("b");

    ParsedChainSegment bodySeg;
    bodySeg.command.keywords.push_back("echo");
    bodySeg.command.arguments.push_back("$i");
    ast.forLoop.body.segments.push_back(bodySeg);

    executor.execute(ast);

    TEST_ASSERT_EQUAL(2, executed.size());
    TEST_ASSERT_EQUAL_STRING("a", executed[0].arguments[0].c_str());
    TEST_ASSERT_EQUAL_STRING("b", executed[1].arguments[0].c_str());
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_executor_runs_simple_chain);
    RUN_TEST(test_executor_respects_andand);
    RUN_TEST(test_executor_respects_oror);
    RUN_TEST(test_executor_expands_for_loop_values);
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

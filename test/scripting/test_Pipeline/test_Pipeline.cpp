#include <unity.h>

#include "Scripting/Runner.h"
#include "../../Mocks/MockTerminal.h"
#include "../../Mocks/MockCommand.h"

using namespace EmbeddedTerminal::Scripting;
using EmbeddedTerminal::token_list_t;
using EmbeddedTerminal::token_t;
using EmbeddedTerminal::TokenType;

class RunnerHarness : public Runner
{
public:
    RunnerHarness(IExecutionContext &ctx) : Runner(ctx) {}

    bool executePipelineExpression(const PipelineExpression &pipeline, ETMap<ETString, ETString> &variables)
    {
        return Runner::executePipelineExpression(pipeline, variables);
    }
};

class ParserHarness : public ScriptParser
{
public:
    Expression buildCurlyBracedPipelineExpression(const token_list_t &tokens, size_t &index)
    {
        return ScriptParser::buildCurlyBracedPipelineExpression(tokens, index);
    }
};

void setUp(void) {}
void tearDown(void) {}

void test_single_command_in_pipeline(void)
{
    MockTerminal terminal;
    RunnerHarness runner(terminal);
    CapturingCommand echoCommand("ok", 0);
    terminal.registerCommand("echo", &echoCommand);
    auto tokens = Lexer().tokenize("{echo ok}");
    size_t index = 0;
    auto pipelineExpr = ParserHarness().buildCurlyBracedPipelineExpression(tokens, index);

    ETMap<ETString, ETString> variables;
    TEST_ASSERT_TRUE(runner.executePipelineExpression(pipelineExpr.data.pipeline, variables));
    TEST_ASSERT_EQUAL_STRING("echo", echoCommand.lastKeyword.c_str());
    TEST_ASSERT_EQUAL_STRING("ok", terminal.getOutput().c_str());
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_single_command_in_pipeline);

    UNITY_END();
}

#if (defined(ESP_PLATFORM) || defined(ESP32)) && !defined(ARDUINO)
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

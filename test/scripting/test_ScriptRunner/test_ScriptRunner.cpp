#include <unity.h>

#include "Terminal.h"
#include "scripting/Runner.h"
#include "../../Mocks/MockStream.h"
#include "../../Mocks/MockCommand.h"

using namespace EmbeddedTerminal;
using namespace EmbeddedTerminal::Scripting;

void setUp(void) {}
void tearDown(void) {}

namespace
{

    class ScriptRunnerHarness : public Runner
    {
    public:
        ScriptRunnerHarness(Terminal &terminal) : Runner(terminal) {}

        // Expose protected methods for testing
        using Runner::executeVariableAssignment;
    };

    class OutputCommand : public ICommand
    {
    public:
        OutputCommand(ETString output, int exitCode = 0) : output_(output), exitCode_(exitCode)
        {
        }

        int callCount = 0;

        ETString usage(const ETString &keyword) const override
        {
            return keyword;
        }

        CommandResult invoke(CommandInvocation &invocation) override
        {
            (void)invocation;
            ++callCount;
            invocation.streams.output.print(output_);
            return CommandResult::completed(exitCode_);
        }

    private:
        ETString output_;
        int exitCode_;
    };

    class StoreVariableCommand : public ICommand
    {
    public:
        ETString usage(const ETString &keyword) const override
        {
            return keyword;
        }

        CommandResult invoke(CommandInvocation &invocation) override
        {
            capturedArguments.push_back(join(invocation.arguments, " "));
            if (!invocation.arguments.empty())
            {
                invocation.context.variables["captured"] = invocation.arguments[0];
            }
            return CommandResult::completed(0);
        }

    public:
        ETVector<ETString> capturedArguments;
    };

}

void test_script_runner_set_variables_and_evaluate_condition(void)
{
    MockStream stream;
    Terminal terminal(stream);
    ScriptRunnerHarness runner(terminal);

    ETMap<ETString, ETString> variables;
    variables["foo"] = "bar";

    token_list_t tokens;
    tokens.push_back({"var0", TokenType::WORD});
    tokens.push_back({"=", TokenType::EQUALS});
    tokens.push_back({"true", TokenType::TRUE});
    tokens.push_back({";", TokenType::SEMI});
    tokens.push_back({"var1", TokenType::WORD});
    tokens.push_back({"=", TokenType::EQUALS});
    tokens.push_back({"false", TokenType::FALSE});
    tokens.push_back({"", TokenType::NEWLINE});

    ScriptParser parser;
    auto ast = parser.parse(tokens);
    TEST_ASSERT_FALSE(parser.error());
    runner.execute(ast, variables);
    TEST_ASSERT_EQUAL(3, variables.size());
    TEST_ASSERT_TRUE(variables.find("var0") != variables.end());
    TEST_ASSERT_TRUE(variables.find("var1") != variables.end());
    TEST_ASSERT_TRUE(variables.find("foo") != variables.end());
    TEST_ASSERT_EQUAL_STRING("true", variables.find("var0")->second.c_str());
    TEST_ASSERT_EQUAL_STRING("false", variables.find("var1")->second.c_str());
    TEST_ASSERT_EQUAL_STRING("bar", variables.find("foo")->second.c_str());
}

void test_script_runner_simple_for_loop(void)
{
    MockStream stream;
    Terminal terminal(stream);
    ScriptRunnerHarness runner(terminal);

    ETMap<ETString, ETString> variables;

    StreamingMockCommandTokenCollector echoCmd;
    terminal.registerCommand("echo", &echoCmd);

    Lexer lexer;
    token_list_t tokens = lexer.tokenize("for i = 1 2 3; do echo $i; done");

    ScriptParser parser;
    auto ast = parser.parse(tokens);
    TEST_ASSERT_FALSE(parser.error());
    runner.execute(ast, variables);
    TEST_ASSERT_FALSE(runner.error());
    TEST_ASSERT_EQUAL(3, echoCmd.collectedTokens.size());
    TEST_ASSERT_EQUAL_STRING("1", echoCmd.collectedTokens[0].c_str());
    TEST_ASSERT_EQUAL_STRING("2", echoCmd.collectedTokens[1].c_str());
    TEST_ASSERT_EQUAL_STRING("3", echoCmd.collectedTokens[2].c_str());
}

void test_script_runner_function_definition_and_call(void)
{
    MockStream stream;
    Terminal terminal(stream);
    ScriptRunnerHarness runner(terminal);
    auto collector = new StreamingMockCommandTokenCollector();
    terminal.registerCommand("collect", collector);

    ETMap<ETString, ETString> variables;

    Lexer lexer;
    token_list_t tokens = lexer.tokenize("function myfunc(i j) collect $i $j done\nmyfunc(1 2)");

    ScriptParser parser;
    auto ast = parser.parse(tokens);
    TEST_ASSERT_FALSE(parser.error());
    runner.execute(ast, variables);
    TEST_ASSERT_FALSE(runner.error());
    TEST_ASSERT_EQUAL(2, collector->collectedTokens.size());
    TEST_ASSERT_EQUAL_STRING("1", collector->collectedTokens[0].c_str());
    TEST_ASSERT_EQUAL_STRING("2", collector->collectedTokens[1].c_str());
}

void test_script_runner_function_definition_and_call_with_variable_arguments(void)
{
    MockStream stream;
    Terminal terminal(stream);
    ScriptRunnerHarness runner(terminal);
    auto collector = new StreamingMockCommandTokenCollector();
    terminal.registerCommand("collect", collector);

    ETMap<ETString, ETString> variables;
    variables["name"] = "bob";

    Lexer lexer;
    token_list_t tokens = lexer.tokenize("function myfunc(i) collect $i done\nmyfunc($name)");

    ScriptParser parser;
    auto ast = parser.parse(tokens);
    TEST_ASSERT_FALSE(parser.error());
    runner.execute(ast, variables);
    TEST_ASSERT_FALSE(runner.error());
    TEST_ASSERT_EQUAL(1, collector->collectedTokens.size());
    TEST_ASSERT_EQUAL_STRING("bob", collector->collectedTokens[0].c_str());
}

void test_script_runner_function_scope_does_not_leak_local_variables(void)
{
    MockStream stream;
    Terminal terminal(stream);
    ScriptRunnerHarness runner(terminal);
    auto collector = new StoreVariableCommand();
    terminal.registerCommand("store", collector);

    ETMap<ETString, ETString> variables;
    variables["outer"] = "keep";

    Lexer lexer;
    token_list_t tokens = lexer.tokenize("function scoped(a) store $a done\nscoped(changed)");

    ScriptParser parser;
    auto ast = parser.parse(tokens);
    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL(Runner::State::Completed, runner.execute(ast, variables));

    TEST_ASSERT_EQUAL_STRING("keep", variables["outer"].c_str());
    TEST_ASSERT_TRUE(variables.find("captured") == variables.end());
    TEST_ASSERT_EQUAL_UINT(1, collector->capturedArguments.size());
    TEST_ASSERT_EQUAL_STRING("changed", collector->capturedArguments[0].c_str());
}

void test_script_runner_large_linear_script_completes(void)
{
    MockStream stream;
    Terminal terminal(stream);
    ScriptRunnerHarness runner(terminal);
    auto collector = new StoreVariableCommand();
    terminal.registerCommand("store", collector);

    ETMap<ETString, ETString> variables;
    variables["value"] = "ok";

    ETString script;
    for (int i = 0; i < 64; ++i)
    {
        if (!script.empty())
        {
            script += "; ";
        }
        script += "store $value";
    }

    Lexer lexer;
    token_list_t tokens = lexer.tokenize(script);

    ScriptParser parser;
    auto ast = parser.parse(tokens);
    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL(Runner::State::Completed, runner.execute(ast, variables));
    TEST_ASSERT_EQUAL_UINT(64, collector->capturedArguments.size());
}

void test_script_runner_execute_variable_assignment_expressions(void)
{
    MockStream stream;
    Terminal terminal(stream);
    ScriptRunnerHarness runner(terminal);

    ETMap<ETString, ETString> variables;

    Lexer lexer;
    token_list_t tokens = lexer.tokenize("var1 = hello\nvar2 = world\nvar3 = \"$var1 $var2\"");

    ScriptParser parser;
    size_t index = 0;
    auto ast = parser.parse(tokens, index);
    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL(3, ast.size());

    TEST_ASSERT_EQUAL(0, variables.size());
    TEST_ASSERT_EQUAL(Runner::State::Completed, runner.execute(ast, variables));
    TEST_ASSERT_EQUAL_STRING("hello", variables["var1"].c_str());
    TEST_ASSERT_EQUAL_STRING("world", variables["var2"].c_str());
    TEST_ASSERT_EQUAL_STRING("hello world", variables["var3"].c_str());
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_script_runner_set_variables_and_evaluate_condition);
    RUN_TEST(test_script_runner_simple_for_loop);
    RUN_TEST(test_script_runner_function_definition_and_call);
    RUN_TEST(test_script_runner_function_definition_and_call_with_variable_arguments);
    RUN_TEST(test_script_runner_function_scope_does_not_leak_local_variables);
    RUN_TEST(test_script_runner_large_linear_script_completes);
    RUN_TEST(test_script_runner_execute_variable_assignment_expressions);
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

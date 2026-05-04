// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif

#include "commands/echo.h"
#include "commands/script.h"
#include "../../Mocks/MockStream.h"
#include "../../Mocks/CommandRuntimeTestUtils.h"

using namespace EmbeddedTerminal;

void setUp(void) {}
void tearDown(void) {}

class TestScriptTerminal
{
public:
    TestScriptTerminal() : terminal_(stream_)
    {
        terminal_.registerCommand("echo", new cmd::echo());
        terminal_.registerCommand("script", new cmd::script(terminal_));
    }

    ~TestScriptTerminal()
    {
        // Terminal does NOT own commands, so we clean them up
        auto &commands = const_cast<ETMap<ETString, ICommand *> &>(terminal_.getCommands());
        for (auto &pair : commands)
        {
            delete pair.second;
        }
        commands.clear();
    }

    MockStream stream_;
    Terminal terminal_;
};

void test_script_multiline_for_loop(void)
{
    TestScriptTerminal test;

    // Test: multiline for loop via script command
    ETString scriptText = "for i in 1 2 3; do echo Item $i; done";
    
    cmd::script &scriptCmd = *(static_cast<cmd::script *>(const_cast<ETMap<ETString, ICommand *> &>(test.terminal_.getCommands()).find("script")->second));

    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(test.stream_, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(test.stream_, TerminalChannel::StdErr);

    CommandInvocation invocation{"script", scriptText, context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = scriptCmd.execute(invocation);

    // Script should complete
    TEST_ASSERT_EQUAL(CommandExecutionState::Completed, result.state);
}

void test_script_usage(void)
{
    cmd::script scriptCmd(*(static_cast<Terminal *>(nullptr)));
    ETString usage = scriptCmd.usage("script");
    TEST_ASSERT_TRUE(usage.find("Execute") != ETString::npos || usage.find("script") != ETString::npos);
}

void test_script_trigger_simple(void)
{
    TestScriptTerminal test;
    cmd::script &scriptCmd = *(new cmd::script(test.terminal_));
    test.terminal_.registerCommand("script", &scriptCmd);

    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(test.stream_, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(test.stream_, TerminalChannel::StdErr);

    // Execute simple script via the command's execute method
    CommandInvocation invocation{"script", "echo hello", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = scriptCmd.execute(invocation);

    // Should complete or return a valid state
    TEST_ASSERT_TRUE(result.state == CommandExecutionState::Completed || result.state == CommandExecutionState::WaitingForInput);
}

void test_script_execute_inline_script(void)
{
    TestScriptTerminal test;
    cmd::script &scriptCmd = *(new cmd::script(test.terminal_));
    test.terminal_.registerCommand("script", &scriptCmd);

    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(test.stream_, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(test.stream_, TerminalChannel::StdErr);

    // Simple inline script: two echo commands
    CommandInvocation invocation{"script", "echo first; echo second", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = scriptCmd.execute(invocation);

    // Script should return completed state
    TEST_ASSERT_EQUAL(CommandExecutionState::Completed, result.state);
}

void test_script_multiline_chained_commands(void)
{
    TestScriptTerminal test;
    cmd::script &scriptCmd = *(new cmd::script(test.terminal_));
    test.terminal_.registerCommand("script", &scriptCmd);

    // Test: chained commands via script
    ETString scriptText = "echo start && echo middle && echo end";
    
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(test.stream_, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(test.stream_, TerminalChannel::StdErr);

    CommandInvocation invocation{"script", scriptText, context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = scriptCmd.execute(invocation);

    // The script should complete successfully
    TEST_ASSERT_EQUAL(CommandExecutionState::Completed, result.state);
}

void test_script_empty_script(void)
{
    TestScriptTerminal test;
    cmd::script &scriptCmd = *(new cmd::script(test.terminal_));

    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(test.stream_, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(test.stream_, TerminalChannel::StdErr);

    // Empty script should be handled gracefully
    CommandInvocation invocation{"script", "", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = scriptCmd.execute(invocation);

    // Should not crash; exit code depends on implementation
    TEST_ASSERT_TRUE(result.state == CommandExecutionState::Completed ||
                     result.state == CommandExecutionState::WaitingForInput);
}

void test_script_for_loop_with_variable_substitution(void)
{
    TestScriptTerminal test;
    cmd::script &scriptCmd = *(new cmd::script(test.terminal_));
    test.terminal_.registerCommand("script", &scriptCmd);

    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(test.stream_, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(test.stream_, TerminalChannel::StdErr);

    // Script with for loop and variable substitution
    ETString scriptText = "for x in a b c; do echo x=$x; done";
    CommandInvocation invocation{"script", scriptText, context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = scriptCmd.execute(invocation);

    TEST_ASSERT_EQUAL(CommandExecutionState::Completed, result.state);
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_script_usage);
    RUN_TEST(test_script_trigger_simple);
    RUN_TEST(test_script_execute_inline_script);
    RUN_TEST(test_script_multiline_chained_commands);
    RUN_TEST(test_script_multiline_for_loop);
    RUN_TEST(test_script_empty_script);
    RUN_TEST(test_script_for_loop_with_variable_substitution);
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

#include <unity.h>
#include "Terminal.h"
#include "../../Mocks/MockStream.h"
#include "interfaces/ICommand.h"

using namespace EmbeddedTerminal;

void setUp(void) {}
void tearDown(void) {}

static bool drainTerminal(Terminal &term, size_t maxSteps = 32)
{
    size_t steps = 0;
    do
    {
        term.loop();
        ++steps;
        if (steps >= maxSteps)
        {
            return false;
        }
    } while (term.isRunning());

    return true;
}

class CallCountCommand : public ICommand
{
public:
    int callCount = 0;

    ETString usage(const ETString &keyword) const override
    {
        return keyword;
    }

    CommandResult invoke(CommandInvocation &invocation) override
    {
        callCount++;
        invocation.streams.output.print("Called " + toETString(callCount) + " times\n");
        return CommandResult::completed(0);
    }
};

class LoopConditionCommand : public ICommand
{
public:
    int callCount = 0;

    ETString usage(const ETString &keyword) const override
    {
        return keyword;
    }

    CommandResult invoke(CommandInvocation &invocation) override
    {
        (void)invocation;
        ++callCount;
        invocation.streams.output.print(callCount <= 3 ? "true" : "false");
        return CommandResult::completed(callCount <= 3 ? 0 : 1);
    }
};

class SuccessCommand : public ICommand
{
public:
    int callCount = 0;

    ETString usage(const ETString &keyword) const override
    {
        return keyword;
    }

    CommandResult invoke(CommandInvocation &invocation) override
    {
        (void)invocation;
        ++callCount;
        invocation.streams.output.print("true");
        return CommandResult::completed(0);
    }
};

class FailureCommand : public ICommand
{
public:
    int callCount = 0;

    ETString usage(const ETString &keyword) const override
    {
        return keyword;
    }

    CommandResult invoke(CommandInvocation &invocation) override
    {
        (void)invocation;
        ++callCount;
        invocation.streams.output.print("false");
        return CommandResult::completed(1);
    }
};

void test_for_loop_execution(void)
{
    MockStream stream;
    Terminal term(stream);
    CallCountCommand cmd;

    term.registerCommand("test", &cmd);

    // In this case a simple command is tryed to be executed with a for loop construct which is not supported in the terminal, so it should raise an error instead of executing the command
    stream.inputBuffer = "for i = hello world; do test $i; done\n";
    // For loop is a script construct, so it should not execute and should raise an error instead
    TEST_ASSERT_TRUE(drainTerminal(term));
    TEST_MESSAGE(("Last exit code: " + toETString(term.getLastExitCode())).c_str());
    TEST_MESSAGE(("Output buffer: " + stream.outputBuffer).c_str());
    TEST_MESSAGE(("Error buffer: " + stream.stderrBuffer).c_str());
    TEST_ASSERT_EQUAL(2, cmd.callCount);                                               // Should not have executed the command at all since for loops are not supported in the terminal
    TEST_ASSERT_EQUAL(TerminalPredefinedResultCodes::SUCCESS, term.getLastExitCode()); // Treats "for" as a command which is not found since script constructs are not supported in the terminal
}

void test_while_loop_execution(void)
{
    MockStream stream;
    Terminal term(stream);
    CallCountCommand cmd;
    LoopConditionCommand condition;

    term.registerCommand("test", &cmd);
    term.registerCommand("cond", &condition);

    // Since while loops are not supported in the terminal, it should raise an error instead of executing the command or condition
    stream.inputBuffer = "cond = { cond }\nwhile $cond; do test loop; cond = { cond }\ndone\n";
    TEST_ASSERT_TRUE(drainTerminal(term));
    TEST_ASSERT_FALSE(term.error());
    TEST_ASSERT_TRUE(term.getVariables().find("cond") != term.getVariables().end());
    TEST_MESSAGE(("Value of cond variable: " + term.getVariables().at("cond")).c_str());
    TEST_ASSERT_EQUAL(3, cmd.callCount);
    TEST_ASSERT_EQUAL(4, condition.callCount);
    TEST_ASSERT_EQUAL(TerminalPredefinedResultCodes::SUCCESS, term.getLastExitCode());
}

void test_terminal_if_condition_execution(void)
{
    MockStream stream;
    Terminal term(stream);
    CallCountCommand cmd;
    SuccessCommand success;
    FailureCommand failure;

    term.registerCommand("test", &cmd);
    term.registerCommand("success", &success);
    term.registerCommand("failure", &failure);

    // Since if conditions are not supported in the terminal, it should raise an error instead of executing the command or condition, and it should not execute any commands inside the if condition to avoid side effects since script constructs are not supported in the terminal
    stream.inputPos = 0;
    stream.inputBuffer = "cond = { failure }\nif $cond; then test should_not_run; fi\n";
    TEST_ASSERT_TRUE(drainTerminal(term));
    TEST_ASSERT_EQUAL(0, cmd.callCount);     // Should not have executed the command since unsupported script constructs should not execute any commands
    TEST_ASSERT_EQUAL(1, failure.callCount); // Should not have executed the failure command to avoid side effects since script constructs are not supported in the terminal
    TEST_ASSERT_EQUAL(TerminalPredefinedResultCodes::SUCCESS, term.getLastExitCode());

    cmd.callCount = 0;
    success.callCount = 0;
    failure.callCount = 0;

    stream.inputPos = 0;
    stream.inputBuffer = "cond = { success }\nif $cond; then test should_run; fi\n";
    TEST_ASSERT_TRUE(drainTerminal(term));
    TEST_ASSERT_EQUAL(1, cmd.callCount);
    TEST_ASSERT_EQUAL(0, failure.callCount);
    TEST_ASSERT_EQUAL(0, term.getLastExitCode());
}

void test_terminal_if_condition_reacts_on_bool(void)
{
    MockStream stream;
    Terminal term(stream);
    CallCountCommand cmd;

    term.registerCommand("test", &cmd);

    // Since if conditions are not supported in the terminal, it should raise an error instead of executing the command or condition, and it should not execute any commands inside the if condition to avoid side effects since script constructs are not supported in the terminal, even if the condition is a simple boolean expression which could be evaluated without executing any commands
    stream.inputPos = 0;
    stream.inputBuffer = "if \"1\"; then test should_run; fi\n";
    TEST_ASSERT_TRUE(drainTerminal(term));
    TEST_ASSERT_EQUAL(1, cmd.callCount);
    TEST_ASSERT_EQUAL(TerminalPredefinedResultCodes::SUCCESS, term.getLastExitCode());

    cmd.callCount = 0;

    stream.inputPos = 0;
    stream.inputBuffer = "if true; then test should_run; fi\n";
    TEST_ASSERT_TRUE(drainTerminal(term));
    TEST_ASSERT_EQUAL(1, cmd.callCount);
    TEST_ASSERT_EQUAL(TerminalPredefinedResultCodes::SUCCESS, term.getLastExitCode());

    cmd.callCount = 0;

    stream.inputPos = 0;
    stream.inputBuffer = "if \"\"; then test should_not_run; fi\n";
    TEST_ASSERT_TRUE(drainTerminal(term));
    TEST_ASSERT_EQUAL(0, cmd.callCount);
    TEST_ASSERT_EQUAL(TerminalPredefinedResultCodes::SUCCESS, term.getLastExitCode());

    cmd.callCount = 0;

    stream.inputPos = 0;
    stream.inputBuffer = "if false; then test should_not_run; fi\n";
    TEST_ASSERT_TRUE(drainTerminal(term));
    TEST_ASSERT_EQUAL(0, cmd.callCount);
    TEST_ASSERT_EQUAL(TerminalPredefinedResultCodes::SUCCESS, term.getLastExitCode());

    cmd.callCount = 0;

    stream.inputPos = 0;
    stream.inputBuffer = "if true || false; then test should_run; fi\n";
    TEST_ASSERT_TRUE(drainTerminal(term));
    TEST_ASSERT_EQUAL(1, cmd.callCount);
    TEST_ASSERT_EQUAL(TerminalPredefinedResultCodes::SUCCESS, term.getLastExitCode());

    cmd.callCount = 0;

    stream.inputPos = 0;
    stream.inputBuffer = "if false || true; then test should_run; fi\n";
    TEST_ASSERT_TRUE(drainTerminal(term));
    TEST_ASSERT_EQUAL(1, cmd.callCount);
    TEST_ASSERT_EQUAL(TerminalPredefinedResultCodes::SUCCESS, term.getLastExitCode());

    cmd.callCount = 0;

    stream.inputPos = 0;
    stream.inputBuffer = "if true && false; then test should_not_run; fi\n";
    TEST_ASSERT_TRUE(drainTerminal(term));
    TEST_ASSERT_EQUAL(0, cmd.callCount);
    TEST_ASSERT_EQUAL(TerminalPredefinedResultCodes::SUCCESS, term.getLastExitCode());

    cmd.callCount = 0;

    stream.inputPos = 0;
    stream.inputBuffer = "if true && true; then test should_run; fi\n";
    TEST_ASSERT_TRUE(drainTerminal(term));
    TEST_ASSERT_EQUAL(1, cmd.callCount);
    TEST_ASSERT_EQUAL(TerminalPredefinedResultCodes::SUCCESS, term.getLastExitCode());
}

void test_after_failing_command_should_not_execute_next_command(void)
{
    MockStream stream;
    Terminal term(stream);
    SuccessCommand success;
    FailureCommand failure;

    term.registerCommand("success", &success);
    // We do not register the failure command to simulate the case where a command is not found which should also cause the terminal to raise an error
    // and not execute any following commands
    // term.registerCommand("failure", &failure);

    // Since script constructs are not supported in the terminal, it should raise an error instead of executing the commands, and it should not execute any commands to avoid side effects since script constructs are not supported in the terminal
    stream.inputPos = 0;
    stream.inputBuffer = "failure; success\n";
    TEST_ASSERT_TRUE(drainTerminal(term));
    TEST_ASSERT_EQUAL(0, success.callCount);
    TEST_ASSERT_EQUAL(0, failure.callCount); // Failure command is not registered, so it should not have executed and should not have caused any side effects
    TEST_ASSERT_EQUAL(TerminalPredefinedResultCodes::COMMAND_NOT_FOUND, term.getLastExitCode());
}

void test_after_failing_command_with_and_should_not_execute_next_command(void)
{
    MockStream stream;
    Terminal term(stream);
    SuccessCommand success;
    FailureCommand failure;

    term.registerCommand("success", &success);
    term.registerCommand("failure", &failure);

    // Since script constructs are not supported in the terminal, it should raise an error instead of executing the commands, and it should not execute any commands to avoid side effects since script constructs are not supported in the terminal
    stream.inputPos = 0;
    stream.inputBuffer = "cond = { failure }\nif $cond && true; then success; fi\n";
    TEST_ASSERT_TRUE(drainTerminal(term));
    TEST_ASSERT_EQUAL(0, success.callCount); // The failing command output should make the condition false, so the body must not run
    TEST_ASSERT_EQUAL(1, failure.callCount); // The command is used through a variable-backed condition and should execute once
    TEST_ASSERT_EQUAL(TerminalPredefinedResultCodes::SUCCESS, term.getLastExitCode());
}

void test_multiple_commands_in_one_line_should_execute_all_commands_one_by_one(void)
{
    MockStream stream;
    Terminal term(stream);
    CallCountCommand cmd1;
    CallCountCommand cmd2;

    term.registerCommand("cmd1", &cmd1);
    term.registerCommand("cmd2", &cmd2);

    stream.inputPos = 0;
    stream.inputBuffer = "cmd1; cmd2\n";
    TEST_ASSERT_TRUE(drainTerminal(term));
    TEST_ASSERT_EQUAL(1, cmd1.callCount);
    TEST_ASSERT_EQUAL(1, cmd2.callCount);
}

void process_tests()
{
    UNITY_BEGIN();
    // Since terminal shall not execute scripts, we test if all the scripting raises an error when attempted to execute from the terminal
    RUN_TEST(test_for_loop_execution);
    RUN_TEST(test_while_loop_execution);
    RUN_TEST(test_terminal_if_condition_execution);
    RUN_TEST(test_terminal_if_condition_reacts_on_bool);
    RUN_TEST(test_after_failing_command_should_not_execute_next_command);
    RUN_TEST(test_after_failing_command_with_and_should_not_execute_next_command);
    RUN_TEST(test_multiple_commands_in_one_line_should_execute_all_commands_one_by_one);

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
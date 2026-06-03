#include <unity.h>

#include "Terminal.h"
#include "scripting/Runner.h"
#include "../../Mocks/MockStream.h"

using namespace EmbeddedTerminal;
using namespace EmbeddedTerminal::Scripting;

class ScriptRunnerHarness : public Runner
{
public:
    ScriptRunnerHarness(Terminal &terminal) : Runner(terminal) {}

    // Expose protected methods for testing
    using Runner::getLastPauseState;
};

class PauseThenWaitCommand : public ICommand
{
public:
    int resumeCount = 0;

    ETString usage(const ETString &keyword) const override { return keyword; }

    CommandResult invoke(CommandInvocation &invocation) override
    {
        invocation.streams.output.print("phase1");
        return CommandResult::running(0);
    }

    CommandResult resume(CommandInvocation &invocation) override
    {
        resumeCount++;
        if (resumeCount == 1)
        {
            invocation.streams.output.print("phase2");
            return CommandResult::waitingForInput(0);
        }
        invocation.streams.output.print("phase3");
        return CommandResult::completed(0);
    }
};

void setUp(void) {}
void tearDown(void) {}

void test_runner_handles_multiple_resume_states(void)
{
    MockStream stream;
    Terminal terminal(stream);
    ScriptRunnerHarness runner(terminal);

    PauseThenWaitCommand cmd;
    terminal.registerCommand("pause", &cmd);

    ETMap<ETString, ETString> variables;
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("pause");

    ScriptParser parser;
    auto ast = parser.parse(tokens);
    TEST_ASSERT_FALSE(parser.error());

    // First execute -> invoke() -> Running
    TEST_ASSERT_EQUAL(Runner::State::Running, runner.execute(ast, variables));
    TEST_ASSERT_EQUAL(CommandExecutionState::Running, runner.getLastPauseState());
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("phase1") != ETString::npos);

    // Second execute -> resume() -> WaitingForInput
    TEST_ASSERT_EQUAL(Runner::State::Running, runner.execute(ast, variables));
    TEST_ASSERT_EQUAL(CommandExecutionState::WaitingForInput, runner.getLastPauseState());
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("phase2") != ETString::npos);

    // Third execute -> resume() -> Completed
    stream.inputBuffer = "input data";
    TEST_ASSERT_EQUAL(Runner::State::Completed, runner.execute(ast, variables));
    TEST_ASSERT_EQUAL(CommandExecutionState::Completed, runner.getLastPauseState());
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("phase3") != ETString::npos);
}

class MultiStepResumableCommand : public ICommand
{
public:
    ETVector<ETString> stateLog;

    ETString usage(const ETString &keyword) const override { return keyword; }

    CommandResult invoke(CommandInvocation &invocation) override
    {
        stateLog.push_back("invoke");
        invocation.streams.output.print("step1:");
        return CommandResult::running(0);
    }

    CommandResult resume(CommandInvocation &invocation) override
    {
        stateLog.push_back("resume");
        if (stateLog.size() == 2)
        {
            invocation.streams.output.print("step2:");
            return CommandResult::running(0);
        }
        if (stateLog.size() == 3)
        {
            invocation.streams.output.print("step3:");
            return CommandResult::running(0);
        }
        invocation.streams.output.print("step4:");
        return CommandResult::completed(0);
    }
};

class ErrorProneCommand : public ICommand
{
public:
    int executeCount = 0;
    ETString usage(const ETString &keyword) const override { return keyword; }

    CommandResult invoke(CommandInvocation &invocation) override
    {
        executeCount++;
        invocation.streams.output.print("attempt1");
        return CommandResult::running(0);
    }

    CommandResult resume(CommandInvocation &invocation) override
    {
        executeCount++;
        if (executeCount == 2)
        {
            invocation.streams.error.print("error_on_resume");
            return CommandResult::completed(42);
        }
        invocation.streams.output.print("attempt2");
        return CommandResult::completed(0);
    }
};

class WaitingInputCommand : public ICommand
{
public:
    ETVector<ETString> inputs;
    size_t inputIndex = 0;

    ETString usage(const ETString &keyword) const override { return keyword; }

    CommandResult invoke(CommandInvocation &invocation) override
    {
        invocation.streams.output.print("start");
        return CommandResult::waitingForInput(0);
    }

    CommandResult resume(CommandInvocation &invocation) override
    {
        if (inputIndex < inputs.size())
        {
            invocation.streams.output.print(inputs[inputIndex]);
            inputIndex++;
            if (inputIndex < inputs.size())
            {
                return CommandResult::waitingForInput(0);
            }
        }
        return CommandResult::completed(0);
    }
};

void test_runner_consecutive_resume_calls(void)
{
    MockStream stream;
    Terminal terminal(stream);
    ScriptRunnerHarness runner(terminal);

    MultiStepResumableCommand cmd;
    terminal.registerCommand("multistep", &cmd);

    ETMap<ETString, ETString> variables;
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("multistep");

    ScriptParser parser;
    auto ast = parser.parse(tokens);
    TEST_ASSERT_FALSE(parser.error());

    TEST_ASSERT_EQUAL(1, cmd.stateLog.size());
    TEST_ASSERT_EQUAL_STRING("invoke", cmd.stateLog[0].c_str());

    TEST_ASSERT_EQUAL(Runner::State::Running, runner.execute(ast, variables));
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("step1") != ETString::npos);
    TEST_ASSERT_EQUAL(2, cmd.stateLog.size());

    TEST_ASSERT_EQUAL(Runner::State::Running, runner.execute(ast, variables));
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("step2") != ETString::npos);
    TEST_ASSERT_EQUAL(3, cmd.stateLog.size());

    TEST_ASSERT_EQUAL(Runner::State::Running, runner.execute(ast, variables));
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("step3") != ETString::npos);
    TEST_ASSERT_EQUAL(4, cmd.stateLog.size());

    TEST_ASSERT_EQUAL(Runner::State::Completed, runner.execute(ast, variables));
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("step4") != ETString::npos);
    TEST_ASSERT_EQUAL(5, cmd.stateLog.size());
}

void test_runner_error_propagation_on_resume(void)
{
    MockStream stream;
    Terminal terminal(stream);
    ScriptRunnerHarness runner(terminal);

    ErrorProneCommand cmd;
    terminal.registerCommand("errorcmd", &cmd);

    ETMap<ETString, ETString> variables;
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("errorcmd");

    ScriptParser parser;
    auto ast = parser.parse(tokens);
    TEST_ASSERT_FALSE(parser.error());

    TEST_ASSERT_EQUAL(1, cmd.executeCount);
    TEST_ASSERT_EQUAL(Runner::State::Running, runner.execute(ast, variables));
    TEST_ASSERT_EQUAL(2, cmd.executeCount);

    TEST_ASSERT_EQUAL(Runner::State::Error, runner.execute(ast, variables));
    TEST_ASSERT_TRUE(stream.stderrBuffer.find("error_on_resume") != ETString::npos);
}

void test_runner_multiple_waiting_for_input_transitions(void)
{
    MockStream stream;
    Terminal terminal(stream);
    ScriptRunnerHarness runner(terminal);

    WaitingInputCommand cmd;
    cmd.inputs.push_back("input1");
    cmd.inputs.push_back("input2");
    terminal.registerCommand("wait", &cmd);

    ETMap<ETString, ETString> variables;
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("wait");

    ScriptParser parser;
    auto ast = parser.parse(tokens);
    TEST_ASSERT_FALSE(parser.error());

    TEST_ASSERT_EQUAL(Runner::State::Running, runner.execute(ast, variables));
    TEST_ASSERT_EQUAL(CommandExecutionState::WaitingForInput, runner.getLastPauseState());
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("start") != ETString::npos);

    TEST_ASSERT_EQUAL(Runner::State::Running, runner.execute(ast, variables));
    TEST_ASSERT_EQUAL(CommandExecutionState::WaitingForInput, runner.getLastPauseState());
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("input1") != ETString::npos);

    TEST_ASSERT_EQUAL(Runner::State::Completed, runner.execute(ast, variables));
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("input2") != ETString::npos);
}

void test_runner_function_scope_with_variable_shadowing(void)
{
    MockStream stream;
    Terminal terminal(stream);
    ScriptRunnerHarness runner(terminal);

    class SetVarCommand : public ICommand
    {
    public:
        ETString varName;
        ETString varValue;
        ETString usage(const ETString &keyword) const override { return keyword; }
        CommandResult invoke(CommandInvocation &invocation) override
        {
            if (invocation.arguments.size() >= 2)
            {
                varName = invocation.arguments[0];
                varValue = invocation.arguments[1];
                invocation.context.variables[varName] = varValue;
            }
            return CommandResult::completed(0);
        }
    };

    SetVarCommand setCmd;
    terminal.registerCommand("set", &setCmd);

    ETMap<ETString, ETString> variables;
    variables["x"] = "global_value";

    Lexer lexer;
    token_list_t tokens = lexer.tokenize("function modify(); do set x local_value; done; modify();");

    ScriptParser parser;
    auto ast = parser.parse(tokens);
    TEST_ASSERT_FALSE(parser.error());

    TEST_ASSERT_EQUAL(Runner::State::Completed, runner.execute(ast, variables));
    TEST_ASSERT_EQUAL_STRING("global_value", variables["x"].c_str());
}

void test_runner_function_local_variables_dont_leak(void)
{
    MockStream stream;
    Terminal terminal(stream);
    ScriptRunnerHarness runner(terminal);

    class NoOpCommand : public ICommand
    {
    public:
        ETString usage(const ETString &keyword) const override { return keyword; }
        CommandResult invoke(CommandInvocation &invocation) override
        {
            return CommandResult::completed(0);
        }
    };

    NoOpCommand noop;
    terminal.registerCommand("noop", &noop);

    ETMap<ETString, ETString> variables;
    variables["existing"] = "value";

    Lexer lexer;
    token_list_t tokens = lexer.tokenize("function newFunc(); do noop; done; newFunc();");

    ScriptParser parser;
    auto ast = parser.parse(tokens);
    TEST_ASSERT_FALSE(parser.error());

    TEST_ASSERT_EQUAL(Runner::State::Completed, runner.execute(ast, variables));
    TEST_ASSERT_EQUAL(1, variables.size());
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_runner_handles_multiple_resume_states);
    RUN_TEST(test_runner_function_scope_with_variable_shadowing);
    RUN_TEST(test_runner_function_local_variables_dont_leak);
    UNITY_END();
    return 0;
}

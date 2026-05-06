// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#endif
#if !defined(ARDUINO) && !defined(ESP_PLATFORM) && !defined(ESP_32)
#include <chrono>
#include <thread>
#endif

#include <unity.h>
#include "../../../src/Terminal.h"
#include "../../../src/commands/script.h"
#include "../../Mocks/MockStream.h"
#include "../../Mocks/native/MockFileSystem.h"

using namespace EmbeddedTerminal;

namespace
{
    class TestInputChannel : public IInputChannel
    {
    public:
        ETString data;
        bool consumed = false;

        bool available() override
        {
            return !consumed && !data.empty();
        }

        ETString readAll() override
        {
            if (consumed)
            {
                return "";
            }

            consumed = true;
            return data;
        }
    };

    class TestOutputChannel : public IOutputChannel
    {
    public:
        ETString buffer;

        void print(const ETString &s) override
        {
            buffer += s;
        }
    };

    class CollectArgsCommand : public ICommand
    {
    public:
        ETVector<ETString> collected;

        ETString usage(const ETString &keyword) override
        {
            return keyword;
        }

        CommandResult execute(CommandInvocation &invocation) override
        {
            collected.push_back(invocation.arguments);
            return CommandResult::completed(0);
        }

    protected:
        ETString trigger(const ETString &keyword, const ETString &additional) override
        {
            (void)keyword;
            (void)additional;
            return "";
        }
    };

    class RunningTwiceCommand : public ICommand
    {
    public:
        int executionCount = 0;

        ETString usage(const ETString &keyword) override
        {
            return keyword;
        }

        CommandResult execute(CommandInvocation &invocation) override
        {
            (void)invocation;
            executionCount++;
            if (executionCount == 1)
            {
                return CommandResult::running(0);
            }

            return CommandResult::completed(0);
        }

    protected:
        ETString trigger(const ETString &keyword, const ETString &additional) override
        {
            (void)keyword;
            (void)additional;
            return "";
        }
    };

    class WaitForInputCommand : public ICommand
    {
    public:
        int executionCount = 0;

        ETString usage(const ETString &keyword) override
        {
            return keyword;
        }

        CommandResult execute(CommandInvocation &invocation) override
        {
            (void)invocation;
            executionCount++;
            if (executionCount == 1)
            {
                return CommandResult::waitingForInput(0);
            }

            return CommandResult::completed(0);
        }

    protected:
        ETString trigger(const ETString &keyword, const ETString &additional) override
        {
            (void)keyword;
            (void)additional;
            return "";
        }
    };

    class FailCommand : public ICommand
    {
    public:
        int executionCount = 0;

        ETString usage(const ETString &keyword) override
        {
            return keyword;
        }

        CommandResult execute(CommandInvocation &invocation) override
        {
            (void)invocation;
            executionCount++;
            return CommandResult::completed(1);
        }

    protected:
        ETString trigger(const ETString &keyword, const ETString &additional) override
        {
            (void)keyword;
            (void)additional;
            return "";
        }
    };
}

void writeScriptFile(MockFileSystem &fs, const ETString &path, const ETString &content)
{
    ETFile file = fs.open(path, FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(file.isOpen());
    TEST_ASSERT_TRUE(file.writeAll(content));
    file.close();
}

void setUp(void) {}
void tearDown(void) {}

void test_script_command_rejects_missing_arguments(void)
{
    MockStream stream;
    Terminal terminal(stream);
    cmd::script scriptCommand(terminal);
    ETMap<ETString, ETString> variables;
    CommandContext context(variables, 0, true);
    TestInputChannel stdinChannel;
    TestOutputChannel stdoutChannel;
    TestOutputChannel stderrChannel;
    ETString keyword = "script";
    ETString arguments = "";
    CommandInvocation invocation{keyword, arguments, context, stdinChannel, stdoutChannel, stderrChannel};

    CommandResult result = scriptCommand.execute(invocation);

    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(2, result.exitCode);
    TEST_ASSERT_TRUE(stderrChannel.buffer.find("Missing required argument") != ETString::npos);
}

void test_script_command_executes_inline_script_cooperatively(void)
{
    MockStream stream;
    Terminal terminal(stream);
    MockFileSystem fs;
    terminal.setFileSystem(&fs);
    cmd::script scriptCommand(terminal);
    CollectArgsCommand collect;
    terminal.registerCommand("collect", &collect);
    ETMap<ETString, ETString> variables;
    CommandContext context(variables, 0, true);
    TestInputChannel stdinChannel;
    TestOutputChannel stdoutChannel;
    TestOutputChannel stderrChannel;
    ETString keyword = "script";
    ETString arguments = "-f /inline.et";
    CommandInvocation invocation{keyword, arguments, context, stdinChannel, stdoutChannel, stderrChannel};

    writeScriptFile(fs, "/inline.et", "collect one; collect two");

    CommandResult result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("one", collect.collected[0].c_str());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(2, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("two", collect.collected[1].c_str());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(0, result.exitCode);
}

void test_script_command_executes_script_file(void)
{
    MockStream stream;
    Terminal terminal(stream);
    MockFileSystem fs;
    terminal.setFileSystem(&fs);
    cmd::script scriptCommand(terminal);
    CollectArgsCommand collect;
    terminal.registerCommand("collect", &collect);
    ETMap<ETString, ETString> variables;
    CommandContext context(variables, 0, true);
    TestInputChannel stdinChannel;
    TestOutputChannel stdoutChannel;
    TestOutputChannel stderrChannel;
    ETString keyword = "script";
    ETString arguments = "-f /demo.et";
    CommandInvocation invocation{keyword, arguments, context, stdinChannel, stdoutChannel, stderrChannel};

    writeScriptFile(fs, "/demo.et", "collect alpha; collect beta");

    CommandResult result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("alpha", collect.collected[0].c_str());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(2, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("beta", collect.collected[1].c_str());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
}

void test_script_command_resumes_running_subcommand(void)
{
    MockStream stream;
    Terminal terminal(stream);
    MockFileSystem fs;
    terminal.setFileSystem(&fs);
    cmd::script scriptCommand(terminal);
    RunningTwiceCommand run;
    CollectArgsCommand collect;
    terminal.registerCommand("run", &run);
    terminal.registerCommand("collect", &collect);
    ETMap<ETString, ETString> variables;
    CommandContext context(variables, 0, true);
    TestInputChannel stdinChannel;
    TestOutputChannel stdoutChannel;
    TestOutputChannel stderrChannel;
    ETString keyword = "script";
    ETString arguments = "-f /resume.et";
    CommandInvocation invocation{keyword, arguments, context, stdinChannel, stdoutChannel, stderrChannel};

    writeScriptFile(fs, "/resume.et", "run; collect done");

    CommandResult result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, run.executionCount);
    TEST_ASSERT_EQUAL(0, collect.collected.size());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(2, run.executionCount);
    TEST_ASSERT_EQUAL(0, collect.collected.size());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("done", collect.collected[0].c_str());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
}

void test_script_command_waits_for_input_before_resuming_subcommand(void)
{
    MockStream stream;
    Terminal terminal(stream);
    MockFileSystem fs;
    terminal.setFileSystem(&fs);
    cmd::script scriptCommand(terminal);
    WaitForInputCommand wait;
    CollectArgsCommand collect;
    terminal.registerCommand("wait", &wait);
    terminal.registerCommand("collect", &collect);
    ETMap<ETString, ETString> variables;
    CommandContext context(variables, 0, true);
    TestInputChannel stdinChannel;
    TestOutputChannel stdoutChannel;
    TestOutputChannel stderrChannel;
    ETString keyword = "script";
    ETString arguments = "-f /wait.et";
    CommandInvocation invocation{keyword, arguments, context, stdinChannel, stdoutChannel, stderrChannel};

    writeScriptFile(fs, "/wait.et", "wait; collect resumed");

    CommandResult result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::WaitingForInput), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, wait.executionCount);
    TEST_ASSERT_EQUAL(0, collect.collected.size());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::WaitingForInput), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, wait.executionCount);
    TEST_ASSERT_EQUAL(0, collect.collected.size());

    stdinChannel.data = "x";
    stream.inputBuffer = "x";
    stream.inputPos = 0;
    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(2, wait.executionCount);
    TEST_ASSERT_EQUAL(0, collect.collected.size());

    stdinChannel.data = "";
    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("resumed", collect.collected[0].c_str());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
}

void test_script_command_executes_if_then_else_true_branch(void)
{
    MockStream stream;
    Terminal terminal(stream);
    MockFileSystem fs;
    terminal.setFileSystem(&fs);
    cmd::script scriptCommand(terminal);
    CollectArgsCommand collect;
    terminal.registerCommand("collect", &collect);
    ETMap<ETString, ETString> variables;
    CommandContext context(variables, 0, true);
    TestInputChannel stdinChannel;
    TestOutputChannel stdoutChannel;
    TestOutputChannel stderrChannel;
    ETString keyword = "script";
    ETString arguments = "-f /if_true.et";
    CommandInvocation invocation{keyword, arguments, context, stdinChannel, stdoutChannel, stderrChannel};

    writeScriptFile(fs, "/if_true.et", "if collect cond; then collect yes; else collect no; fi");

    CommandResult result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("cond", collect.collected[0].c_str());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(2, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("yes", collect.collected[1].c_str());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(0, result.exitCode);
}

void test_script_command_executes_if_else_false_branch(void)
{
    MockStream stream;
    Terminal terminal(stream);
    MockFileSystem fs;
    terminal.setFileSystem(&fs);
    cmd::script scriptCommand(terminal);
    FailCommand fail;
    CollectArgsCommand collect;
    terminal.registerCommand("fail", &fail);
    terminal.registerCommand("collect", &collect);
    ETMap<ETString, ETString> variables;
    CommandContext context(variables, 0, true);
    TestInputChannel stdinChannel;
    TestOutputChannel stdoutChannel;
    TestOutputChannel stderrChannel;
    ETString keyword = "script";
    ETString arguments = "-f /if_false.et";
    CommandInvocation invocation{keyword, arguments, context, stdinChannel, stdoutChannel, stderrChannel};

    writeScriptFile(fs, "/if_false.et", "if fail; then collect yes; else collect no; fi");

    CommandResult result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, fail.executionCount);
    TEST_ASSERT_EQUAL(0, collect.collected.size());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("no", collect.collected[0].c_str());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
}

void test_script_command_if_without_else_completes_when_condition_false(void)
{
    MockStream stream;
    Terminal terminal(stream);
    MockFileSystem fs;
    terminal.setFileSystem(&fs);
    cmd::script scriptCommand(terminal);
    FailCommand fail;
    CollectArgsCommand collect;
    terminal.registerCommand("fail", &fail);
    terminal.registerCommand("collect", &collect);
    ETMap<ETString, ETString> variables;
    CommandContext context(variables, 0, true);
    TestInputChannel stdinChannel;
    TestOutputChannel stdoutChannel;
    TestOutputChannel stderrChannel;
    ETString keyword = "script";
    ETString arguments = "-f /if_no_else.et";
    CommandInvocation invocation{keyword, arguments, context, stdinChannel, stdoutChannel, stderrChannel};

    writeScriptFile(fs, "/if_no_else.et", "if fail; then collect yes; fi");

    CommandResult result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, fail.executionCount);
    TEST_ASSERT_EQUAL(0, collect.collected.size());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, result.exitCode);
    TEST_ASSERT_EQUAL(0, collect.collected.size());
}

void test_script_command_executes_elif_branch(void)
{
    MockStream stream;
    Terminal terminal(stream);
    MockFileSystem fs;
    terminal.setFileSystem(&fs);
    cmd::script scriptCommand(terminal);
    FailCommand fail;
    CollectArgsCommand collect;
    terminal.registerCommand("fail", &fail);
    terminal.registerCommand("collect", &collect);
    ETMap<ETString, ETString> variables;
    CommandContext context(variables, 0, true);
    TestInputChannel stdinChannel;
    TestOutputChannel stdoutChannel;
    TestOutputChannel stderrChannel;
    ETString keyword = "script";
    ETString arguments = "-f /elif.et";
    CommandInvocation invocation{keyword, arguments, context, stdinChannel, stdoutChannel, stderrChannel};

    writeScriptFile(fs, "/elif.et", "if fail; then collect no; elif collect cond; then collect yes; else collect fallback; fi");

    CommandResult result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, fail.executionCount);
    TEST_ASSERT_EQUAL(0, collect.collected.size());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("cond", collect.collected[0].c_str());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(2, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("yes", collect.collected[1].c_str());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
}

void test_script_command_calls_user_defined_function(void)
{
    MockStream stream;
    Terminal terminal(stream);
    MockFileSystem fs;
    terminal.setFileSystem(&fs);
    cmd::script scriptCommand(terminal);
    CollectArgsCommand collect;
    terminal.registerCommand("collect", &collect);
    ETMap<ETString, ETString> variables;
    CommandContext context(variables, 0, true);
    TestInputChannel stdinChannel;
    TestOutputChannel stdoutChannel;
    TestOutputChannel stderrChannel;
    ETString keyword = "script";
    ETString arguments = "-f /function.et";
    CommandInvocation invocation{keyword, arguments, context, stdinChannel, stdoutChannel, stderrChannel};

    writeScriptFile(fs, "/function.et", "function greet; do collect hello; done; greet; greet");

    CommandResult result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("hello", collect.collected[0].c_str());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(2, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("hello", collect.collected[1].c_str());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(0, result.exitCode);
}

void test_script_command_function_multi_step_body(void)
{
    MockStream stream;
    Terminal terminal(stream);
    MockFileSystem fs;
    terminal.setFileSystem(&fs);
    cmd::script scriptCommand(terminal);
    CollectArgsCommand collect;
    terminal.registerCommand("collect", &collect);
    ETMap<ETString, ETString> variables;
    CommandContext context(variables, 0, true);
    TestInputChannel stdinChannel;
    TestOutputChannel stdoutChannel;
    TestOutputChannel stderrChannel;
    ETString keyword = "script";
    ETString arguments = "-f /function_multi.et";
    CommandInvocation invocation{keyword, arguments, context, stdinChannel, stdoutChannel, stderrChannel};

    writeScriptFile(fs, "/function_multi.et", "function greet; do collect a; collect b; done; greet");

    CommandResult result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("a", collect.collected[0].c_str());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(2, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("b", collect.collected[1].c_str());

    result = scriptCommand.execute(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_script_command_rejects_missing_arguments);
    RUN_TEST(test_script_command_executes_inline_script_cooperatively);
    RUN_TEST(test_script_command_executes_script_file);
    RUN_TEST(test_script_command_resumes_running_subcommand);
    RUN_TEST(test_script_command_waits_for_input_before_resuming_subcommand);
    RUN_TEST(test_script_command_executes_if_then_else_true_branch);
    RUN_TEST(test_script_command_executes_if_else_false_branch);
    RUN_TEST(test_script_command_if_without_else_completes_when_condition_false);
    RUN_TEST(test_script_command_executes_elif_branch);
    RUN_TEST(test_script_command_calls_user_defined_function);
    RUN_TEST(test_script_command_function_multi_step_body);
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

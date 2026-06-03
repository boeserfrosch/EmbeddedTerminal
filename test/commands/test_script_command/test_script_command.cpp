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
#include "../../Mocks/native/MockFileSystem.h"
#include "../utils.h"

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

        ETString usage(const ETString &keyword) const override
        {
            return keyword;
        }

        CommandResult invoke(CommandInvocation &invocation) override
        {
            for (const auto &arg : invocation.arguments)
            {
                collected.push_back(arg);
            }
            invocation.streams.output.print(join(invocation.arguments, " ") + "\n");
            return CommandResult::completed(0);
        }
    };

    class RunningTwiceCommand : public ICommand
    {
    public:
        int executionCount = 0;

        ETString usage(const ETString &keyword) const override
        {
            return keyword;
        }

        CommandResult invoke(CommandInvocation &invocation) override
        {
            executionCount++;
            if (executionCount == 1)
            {
                return CommandResult::running(0);
            }

            return CommandResult::completed(0);
        }

        CommandResult resume(CommandInvocation &invocation) override
        {
            (void)invocation;
            executionCount++;
            if (executionCount == 2)
            {
                return CommandResult::running(0);
            }

            return CommandResult::completed(0);
        }
    };

    class WaitForInputCommand : public ICommand
    {
    public:
        int executionCount = 0;

        WaitForInputCommand(const ETString &inputToWaitFor = "input")
            : inputToWaitFor_(inputToWaitFor)
        {
        }

        ETString usage(const ETString &keyword) const override
        {
            return keyword;
        }

        CommandResult invoke(CommandInvocation &invocation) override
        {
            (void)invocation;
            executionCount++;
            return CommandResult::waitingForInput(0);
        }

        CommandResult resume(CommandInvocation &invocation) override
        {
            if (!invocation.streams.input.available())
            {
                return CommandResult::waitingForInput(0);
            }

            ETString input = invocation.streams.input.readAll();
            if (input == inputToWaitFor_)
            {
                executionCount++;
                return CommandResult::completed(0);
            }

            return CommandResult::waitingForInput(0);
        }
        ETString inputToWaitFor_;
    };

    class FailCommand : public ICommand
    {
    public:
        int executionCount = 0;

        ETString usage(const ETString &keyword) const override
        {
            return keyword;
        }

        CommandResult invoke(CommandInvocation &invocation) override
        {
            (void)invocation;
            executionCount++;
            return CommandResult::completed(1);
        }
    };

    class ScriptCommandProbe : public cmd::script
    {
    public:
        using cmd::script::getErrorMessage_;
        using cmd::script::script;
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
    TestInputChannel input;
    TestOutputChannel output;
    TestOutputChannel error;
    ETString keyword = "script";
    ETVector<ETString> arguments = {};
    CommandInvocation invocation{keyword, arguments, context, input, output, error};

    CommandResult result;
    result = scriptCommand.invoke(invocation);

    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(cmd::script::ErrorCode::InvalidArguments, result.exitCode);
    TEST_ASSERT_TRUE(error.buffer.find(scriptCommand.usage(keyword)) != ETString::npos);
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
    TestInputChannel input;
    TestOutputChannel output;
    TestOutputChannel error;
    ETString keyword = "script";
    ETVector<ETString> arguments = {"/inline.et"};
    CommandInvocation invocation{keyword, arguments, context, input, output, error};

    writeScriptFile(fs, "/inline.et", "collect one; collect two");

    CommandResult result = scriptCommand.invoke(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(2, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("one", collect.collected[0].c_str());
    TEST_ASSERT_EQUAL_STRING("two", collect.collected[1].c_str());
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
    TestInputChannel input;
    TestOutputChannel output;
    TestOutputChannel error;
    ETString keyword = "script";
    ETVector<ETString> arguments = {"/demo.et"};
    CommandInvocation invocation{keyword, arguments, context, input, output, error};

    writeScriptFile(fs, "/demo.et", "collect alpha; collect beta");

    CommandResult result = scriptCommand.invoke(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));

    TEST_ASSERT_EQUAL(2, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("alpha", collect.collected[0].c_str());
    TEST_ASSERT_EQUAL_STRING("beta", collect.collected[1].c_str());
}

void test_script_command_reads_session_variables_without_leaking_writes(void)
{
    MockStream stream;
    Terminal terminal(stream);
    MockFileSystem fs;
    terminal.setFileSystem(&fs);
    cmd::script scriptCommand(terminal);
    CollectArgsCommand collect;
    terminal.registerCommand("collect", &collect);

    ETMap<ETString, ETString> variables;
    variables["name"] = "terminal";
    CommandContext context(variables, 0, true);
    TestInputChannel input;
    TestOutputChannel output;
    TestOutputChannel error;
    ETString keyword = "script";
    ETVector<ETString> arguments = {"/vars.et"};
    CommandInvocation invocation{keyword, arguments, context, input, output, error};

    writeScriptFile(fs, "/vars.et", "collect $name");

    CommandResult result = scriptCommand.invoke(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("terminal", collect.collected[0].c_str());
    TEST_ASSERT_EQUAL_STRING("terminal", variables["name"].c_str());
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

    TestCommandInvocationHandle iHandle("script", {"/resume.et"});
    ;

    writeScriptFile(fs, "/resume.et", "run; collect done");

    CommandResult result = scriptCommand.invoke(iHandle.invocation);
    TEST_ASSERT_EQUAL(CommandExecutionState::Running, result.state); // The script should return Running after executing the "run" command one time, because that command is designed to return Running on the first execution and only complete on the second execution
    TEST_ASSERT_EQUAL(1, run.executionCount);
    CommandResult resumeRunCmdResult = scriptCommand.resume(iHandle.invocation);
    TEST_ASSERT_EQUAL(CommandExecutionState::Running, resumeRunCmdResult.state); // The script steps one expression per resume, so the script is still running after the resumed command finishes
    TEST_ASSERT_EQUAL(2, run.executionCount);
    CommandResult resumeResult = scriptCommand.resume(iHandle.invocation);
    TEST_ASSERT_EQUAL(CommandExecutionState::Running, resumeResult.state); // The next resume advances to the following command, so the script is still running here

    TEST_MESSAGE(iHandle.error);
    TEST_MESSAGE(iHandle.output);
    TEST_ASSERT_EQUAL(3, run.executionCount);
    TEST_ASSERT_EQUAL(0, collect.collected.size());

    resumeResult = scriptCommand.resume(iHandle.invocation);
    TEST_ASSERT_EQUAL(CommandExecutionState::Completed, resumeResult.state);
    TEST_ASSERT_EQUAL(3, run.executionCount);
    TEST_ASSERT_EQUAL(1, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("done", collect.collected[0].c_str());
}

void test_script_command_waits_for_input_before_resuming_subcommand(void)
{
    MockStream stream;
    Terminal terminal(stream);
    MockFileSystem fs;
    terminal.setFileSystem(&fs);
    cmd::script scriptCommand(terminal);
    WaitForInputCommand wait("x");
    CollectArgsCommand collect;
    terminal.registerCommand("wait", &wait);
    terminal.registerCommand("collect", &collect);
    ETMap<ETString, ETString> variables;
    CommandContext context(variables, 0, true);
    TestInputChannel input;
    TestOutputChannel output;
    TestOutputChannel error;
    ETString keyword = "script";
    ETVector<ETString> arguments = {"/wait.et"};
    CommandInvocation invocation{keyword, arguments, context, input, output, error};

    writeScriptFile(fs, "/wait.et", "wait; collect resumed");

    CommandResult result = scriptCommand.invoke(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::WaitingForInput), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, wait.executionCount);
    TEST_ASSERT_EQUAL(0, collect.collected.size());

    result = scriptCommand.resume(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::WaitingForInput), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, wait.executionCount);
    TEST_ASSERT_EQUAL(0, collect.collected.size());

    input.data = "x";
    stream.inputBuffer = "x";
    stream.inputPos = 0;
    result = scriptCommand.resume(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Running), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(2, wait.executionCount);
    TEST_ASSERT_EQUAL(0, collect.collected.size());

    input.data = "";
    result = scriptCommand.resume(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(2, wait.executionCount);
    TEST_ASSERT_EQUAL(1, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("resumed", collect.collected[0].c_str());
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
    TestInputChannel input;
    TestOutputChannel output;
    TestOutputChannel error;
    ETString keyword = "script";
    ETVector<ETString> arguments = {"/if_true.et"};
    CommandInvocation invocation{keyword, arguments, context, input, output, error};

    writeScriptFile(fs, "/if_true.et", "cond = {collect cond}; if $cond; then collect yes; else collect no; fi");

    CommandResult result = scriptCommand.invoke(invocation);

    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(2, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("cond", collect.collected[0].c_str());
    TEST_ASSERT_EQUAL_STRING("yes", collect.collected[1].c_str());
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
    TestInputChannel input;
    TestOutputChannel output;
    TestOutputChannel error;
    ETString keyword = "script";
    ETVector<ETString> arguments = {"/if_false.et"};
    CommandInvocation invocation{keyword, arguments, context, input, output, error};

    writeScriptFile(fs, "/if_false.et", "fail = {fail}; if $fail; then collect yes; else collect no; fi");

    CommandResult result = scriptCommand.invoke(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));

    TEST_ASSERT_EQUAL(1, fail.executionCount);
    TEST_ASSERT_EQUAL(1, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("no", collect.collected[0].c_str());
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
    TestInputChannel input;
    TestOutputChannel output;
    TestOutputChannel error;
    ETString keyword = "script";
    ETVector<ETString> arguments = {"/if_no_else.et"};
    CommandInvocation invocation{keyword, arguments, context, input, output, error};

    writeScriptFile(fs, "/if_no_else.et", "fail = {fail}; if $fail; then collect yes; fi");

    CommandResult result = scriptCommand.invoke(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, fail.executionCount);
    TEST_ASSERT_EQUAL(0, result.exitCode);
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
    TestInputChannel input;
    TestOutputChannel output;
    TestOutputChannel error;
    ETString keyword = "script";
    ETVector<ETString> arguments = {"/elif.et"};
    CommandInvocation invocation{keyword, arguments, context, input, output, error};

    writeScriptFile(fs, "/elif.et", "fail = {fail}; elifCond = {collect cond}; if  $fail; then collect no; elif $elifCond; then collect yes; else collect fallback; fi");

    CommandResult result = scriptCommand.invoke(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(1, fail.executionCount);
    TEST_ASSERT_EQUAL(2, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("cond", collect.collected[0].c_str());
    TEST_ASSERT_EQUAL_STRING("yes", collect.collected[1].c_str());

    result = scriptCommand.invoke(invocation);
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
    TestInputChannel input;
    TestOutputChannel output;
    TestOutputChannel error;
    ETString keyword = "script";
    ETVector<ETString> arguments = {"/function.et"};
    CommandInvocation invocation{keyword, arguments, context, input, output, error};

    writeScriptFile(fs, "/function.et", "function greet(); do collect hello; done; greet(); greet();");

    CommandResult result = scriptCommand.invoke(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(2, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("hello", collect.collected[0].c_str());
    TEST_ASSERT_EQUAL_STRING("hello", collect.collected[1].c_str());
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
    TestInputChannel input;
    TestOutputChannel output;
    TestOutputChannel error;
    ETString keyword = "script";
    ETVector<ETString> arguments = {"/function_multi.et"};
    CommandInvocation invocation{keyword, arguments, context, input, output, error};

    writeScriptFile(fs, "/function_multi.et", "function greet(); do collect a; collect b; done; greet(); greet();");

    CommandResult result = scriptCommand.invoke(invocation);
    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(4, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("a", collect.collected[0].c_str());
    TEST_ASSERT_EQUAL_STRING("b", collect.collected[1].c_str());
    TEST_ASSERT_EQUAL_STRING("a", collect.collected[2].c_str());
    TEST_ASSERT_EQUAL_STRING("b", collect.collected[3].c_str());
}

void test_script_command_usage_and_error_messages(void)
{
    MockStream stream;
    Terminal terminal(stream);
    ScriptCommandProbe scriptCommand(terminal);

    TEST_ASSERT_EQUAL_STRING("script <path> - Execute script from file\n", scriptCommand.usage("script").c_str());
    TEST_ASSERT_EQUAL_STRING("No error", scriptCommand.getErrorMessage_(cmd::script::ErrorCode::None).c_str());
    TEST_ASSERT_EQUAL_STRING("Invalid arguments", scriptCommand.getErrorMessage_(cmd::script::ErrorCode::InvalidArguments).c_str());
    TEST_ASSERT_EQUAL_STRING("File not found", scriptCommand.getErrorMessage_(cmd::script::ErrorCode::FileNotFound).c_str());
    TEST_ASSERT_EQUAL_STRING("Filesystem not available", scriptCommand.getErrorMessage_(cmd::script::ErrorCode::FilesystemNotAvailable).c_str());
    TEST_ASSERT_EQUAL_STRING("File error", scriptCommand.getErrorMessage_(cmd::script::ErrorCode::FileError).c_str());
    TEST_ASSERT_EQUAL_STRING("Parse error", scriptCommand.getErrorMessage_(cmd::script::ErrorCode::ParseError).c_str());
    TEST_ASSERT_EQUAL_STRING("Runtime error", scriptCommand.getErrorMessage_(cmd::script::ErrorCode::RuntimeError).c_str());
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_script_command_rejects_missing_arguments);
    RUN_TEST(test_script_command_executes_inline_script_cooperatively);
    RUN_TEST(test_script_command_executes_script_file);
    RUN_TEST(test_script_command_reads_session_variables_without_leaking_writes);
    RUN_TEST(test_script_command_resumes_running_subcommand);
    RUN_TEST(test_script_command_waits_for_input_before_resuming_subcommand);
    RUN_TEST(test_script_command_executes_if_then_else_true_branch);
    RUN_TEST(test_script_command_executes_if_else_false_branch);
    RUN_TEST(test_script_command_if_without_else_completes_when_condition_false);
    RUN_TEST(test_script_command_executes_elif_branch);
    RUN_TEST(test_script_command_calls_user_defined_function);
    RUN_TEST(test_script_command_function_multi_step_body);
    RUN_TEST(test_script_command_usage_and_error_messages);
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

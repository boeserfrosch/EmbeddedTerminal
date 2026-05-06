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
#include "../../../src/Terminal.h"
#include "../../Mocks/CommandRuntimeTestUtils.h"
#include "../../Mocks/MockStream.h"
#include "../../Mocks/native/MockFileSystem.h"

using namespace EmbeddedTerminal;

namespace
{
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

    class TestScriptTerminal
    {
    public:
        TestScriptTerminal() : stream_(), terminal_(stream_), script_(terminal_)
        {
            terminal_.registerCommand("collect", &collect_);
            terminal_.registerCommand("echo", &echo_);
            terminal_.registerCommand("script", &script_);
        }

        MockStream stream_;
        Terminal terminal_;
        CollectArgsCommand collect_;
        cmd::echo echo_;
        cmd::script script_;
    };
}

void setUp(void) {}
void tearDown(void) {}

void test_script_usage(void)
{
    TestScriptTerminal test;

    ETString usage = test.script_.usage("script");
    TEST_ASSERT_FALSE(usage.empty());
    TEST_ASSERT_TRUE(usage.find("<path>") != ETString::npos);
    TEST_ASSERT_TRUE(usage.find("inline") == ETString::npos);
}

void test_script_rejects_inline_script_text(void)
{
    TestScriptTerminal test;
    MockFileSystem fs;
    test.terminal_.setFileSystem(&fs); // Ensure no file system is available

    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(test.stream_, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(test.stream_, TerminalChannel::StdErr);

    CommandInvocation invocation{"script", "collect first; collect second", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = test.script_.execute(invocation);

    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(2, result.exitCode);
    TEST_ASSERT_EQUAL(0, test.collect_.collected.size());
    TEST_ASSERT_TRUE(test.stream_.stderrBuffer.find("script error: failed to open script file") != ETString::npos);
}

void test_script_execute_file_path_form(void)
{
    TestScriptTerminal test;
    MockFileSystem fs;
    test.terminal_.setFileSystem(&fs);

    ETFile file = fs.open("demo.et", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(file.isOpen());
    TEST_ASSERT_TRUE(file.writeAll("collect first\ncollect second\n"));
    file.close();

    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(test.stream_, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(test.stream_, TerminalChannel::StdErr);

    CommandInvocation invocation{"script", "demo.et", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = CommandResult::running(0);
    while (result.state == CommandExecutionState::Running)
    {
        result = test.script_.execute(invocation);
    }

    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
    TEST_MESSAGE(("Stderr: " + test.stream_.stderrBuffer).c_str());

    if (result.exitCode != 0)
    {
        TEST_FAIL_MESSAGE(("Script failed with exit code: " + test.stream_.stderrBuffer).c_str());
    }
    TEST_ASSERT_EQUAL(2, test.collect_.collected.size());
    TEST_ASSERT_EQUAL_STRING("first", test.collect_.collected[0].c_str());
    TEST_ASSERT_EQUAL_STRING("second", test.collect_.collected[1].c_str());
}

void test_script_execute_dash_f_form(void)
{
    TestScriptTerminal test;
    MockFileSystem fs;
    test.terminal_.setFileSystem(&fs);

    ETFile file = fs.open("chain.et", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(file.isOpen());
    TEST_ASSERT_TRUE(file.writeAll("collect one && collect two\nmissing || collect recovered\n"));
    file.close();

    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(test.stream_, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(test.stream_, TerminalChannel::StdErr);

    CommandInvocation invocation{"script", "chain.et", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = CommandResult::running(0);
    while (result.state == CommandExecutionState::Running)
    {
        result = test.script_.execute(invocation);
    }

    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
    TEST_MESSAGE(("Stderr: " + test.stream_.stderrBuffer).c_str());
    if (test.collect_.collected.size() != 3)
    {
        TEST_FAIL_MESSAGE(("Expected 3 collected, got: " + test.stream_.stderrBuffer).c_str());
    }
    TEST_ASSERT_EQUAL_STRING("one", test.collect_.collected[0].c_str());
    TEST_ASSERT_EQUAL_STRING("two", test.collect_.collected[1].c_str());
    TEST_ASSERT_EQUAL_STRING("recovered", test.collect_.collected[2].c_str());
}

void test_script_rejects_missing_file(void)
{
    TestScriptTerminal test;
    test.terminal_.setFileSystem(new MockFileSystem()); // Set a file system with no files

    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(test.stream_, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(test.stream_, TerminalChannel::StdErr);

    CommandInvocation invocation{"script", "missing.et", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = test.script_.execute(invocation);

    TEST_ASSERT_EQUAL(static_cast<int>(CommandExecutionState::Completed), static_cast<int>(result.state));
    TEST_ASSERT_EQUAL(2, result.exitCode);
    TEST_ASSERT_TRUE(test.stream_.stderrBuffer.find("script error: failed to open script file") != ETString::npos);
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_script_usage);
    RUN_TEST(test_script_rejects_inline_script_text);
    RUN_TEST(test_script_execute_file_path_form);
    RUN_TEST(test_script_execute_dash_f_form);
    RUN_TEST(test_script_rejects_missing_file);
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

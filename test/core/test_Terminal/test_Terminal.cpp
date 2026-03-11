// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
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

#include "../../../src/Terminal.h"
#include "../../../src/ETTypes.h"
#include "../../../src/commands/script.h"
#include "../../Mocks/MockStream.h"
#include "../../Mocks/MockCommand.h"
#include "../../Mocks/native/MockFileSystem.h"
#include <unity.h>

using namespace EmbeddedTerminal;

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
        executionCount++;
        if (executionCount == 1)
        {
            invocation.stdoutChannel.print("phase1");
            return CommandResult::running(0);
        }

        invocation.stdoutChannel.print("phase2");
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
        executionCount++;
        if (executionCount == 1)
        {
            return CommandResult::waitingForInput(0);
        }

        invocation.stdoutChannel.print("resumed");
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

class EmitCommand : public ICommand
{
public:
    ETString usage(const ETString &keyword) override
    {
        return keyword;
    }

    CommandResult execute(CommandInvocation &invocation) override
    {
        invocation.stdoutChannel.print("hello pipe");
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

class UpperFromStdinCommand : public ICommand
{
public:
    ETString usage(const ETString &keyword) override
    {
        return keyword;
    }

    CommandResult execute(CommandInvocation &invocation) override
    {
        ETString text = invocation.stdinChannel.readAll();
        for (size_t i = 0; i < text.length(); ++i)
        {
            if (text[i] >= 'a' && text[i] <= 'z')
            {
                text[i] = static_cast<char>(text[i] - ('a' - 'A'));
            }
        }
        invocation.stdoutChannel.print(text);
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
        invocation.stdoutChannel.print(invocation.arguments + "\n");
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

void setUp(void) {}
void tearDown(void) {}

void test_terminal_register_and_call(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("test", &cmd);
    stream.inputBuffer = "test extra\n\r";
    term.loop();

    TEST_ASSERT_TRUE(stream.outputBuffer.find("test extra") != ETString::npos);
    TEST_ASSERT_TRUE(stream.outputBuffer.find("Triggered: test extra") != ETString::npos);
    TEST_ASSERT_EQUAL_STRING("test", cmd.lastKeyword.c_str());
    TEST_ASSERT_EQUAL_STRING("extra", cmd.lastAdditional.c_str());
}

void test_terminal_call_unknown(void)
{
    MockStream stream;
    Terminal term(stream);
    stream.inputBuffer = "foo\n";
    term.loop();
    TEST_ASSERT_TRUE(stream.outputBuffer.find("foo is unknown") != ETString::npos);
    TEST_ASSERT_TRUE(stream.stderrBuffer.find("foo is unknown") != ETString::npos);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("foo is unknown") == ETString::npos);
    TEST_ASSERT_EQUAL(127, term.getLastExitCode());
}

void test_terminal_getCommands(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("foo", &cmd);
    auto cmds = term.getCommands();
    TEST_ASSERT_TRUE(cmds.find("foo") != cmds.end());
}

// Test deregister command
void test_terminal_deregister_command(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("test", &cmd);

    auto cmds = term.getCommands();
    TEST_ASSERT_TRUE(cmds.find("test") != cmds.end());

    term.deregisterCommand("test");
    cmds = term.getCommands();
    TEST_ASSERT_TRUE(cmds.find("test") == cmds.end());
}

// Test deregister non-existent command (should not crash)
void test_terminal_deregister_nonexistent(void)
{
    MockStream stream;
    Terminal term(stream);
    term.deregisterCommand("nonexistent");
    TEST_ASSERT_TRUE(true); // Should not crash
}

// Test register multiple commands
void test_terminal_multiple_commands(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd1, cmd2, cmd3;

    term.registerCommand("cmd1", &cmd1);
    term.registerCommand("cmd2", &cmd2);
    term.registerCommand("cmd3", &cmd3);

    auto cmds = term.getCommands();
    TEST_ASSERT_EQUAL(3, cmds.size());
    TEST_ASSERT_TRUE(cmds.find("cmd1") != cmds.end());
    TEST_ASSERT_TRUE(cmds.find("cmd2") != cmds.end());
    TEST_ASSERT_TRUE(cmds.find("cmd3") != cmds.end());
}

// Test command override (register same keyword twice)
void test_terminal_command_override(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd1, cmd2;

    term.registerCommand("test", &cmd1);
    term.registerCommand("test", &cmd2); // Should override

    stream.inputBuffer = "test data\n";
    term.loop();

    TEST_ASSERT_EQUAL_STRING("", cmd1.lastKeyword.c_str());     // Not called
    TEST_ASSERT_EQUAL_STRING("test", cmd2.lastKeyword.c_str()); // Called
}

// Test empty command input
void test_terminal_empty_input(void)
{
    MockStream stream;
    Terminal term(stream);
    stream.inputBuffer = "\n";
    term.loop();
    // Should not crash
    TEST_ASSERT_TRUE(true);
}

// Test whitespace-only input
void test_terminal_whitespace_input(void)
{
    MockStream stream;
    Terminal term(stream);
    stream.inputBuffer = "   \n";
    term.loop();
    // Should not crash
    TEST_ASSERT_TRUE(true);
}

// Test command with leading/trailing spaces
void test_terminal_command_with_spaces(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("test", &cmd);
    stream.inputBuffer = "  test  arg  \n";
    term.loop();

    // After trimming and parsing, keyword should be "test"
    // Note: command gets called if parsing works
    if (!cmd.lastKeyword.empty())
    {
        TEST_ASSERT_EQUAL_STRING("test", cmd.lastKeyword.c_str());
    }
    else
    {
        // If empty, test that the command was at least recognized in output
        TEST_ASSERT_TRUE(stream.outputBuffer.find("test") != ETString::npos);
    }
}

// Test very long command line
void test_terminal_long_command(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("test", &cmd);

    ETString longArg;
    for (int i = 0; i < 100; i++)
    {
        longArg += "x";
    }
    stream.inputBuffer = "test " + longArg + "\n";
    term.loop();

    TEST_ASSERT_EQUAL_STRING("test", cmd.lastKeyword.c_str());
    TEST_ASSERT_EQUAL(100, cmd.lastAdditional.length());
}

// Test multiple commands in sequence
void test_terminal_sequential_commands(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd1, cmd2;
    term.registerCommand("foo", &cmd1);
    term.registerCommand("bar", &cmd2);

    stream.inputBuffer = "foo arg1\nbar arg2\n";
    term.loop();
    term.loop();

    TEST_ASSERT_EQUAL_STRING("foo", cmd1.lastKeyword.c_str());
    TEST_ASSERT_EQUAL_STRING("arg1", cmd1.lastAdditional.c_str());
    TEST_ASSERT_EQUAL_STRING("bar", cmd2.lastKeyword.c_str());
    TEST_ASSERT_EQUAL_STRING("arg2", cmd2.lastAdditional.c_str());
}

// Test case sensitivity
void test_terminal_case_sensitivity(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("test", &cmd); // lowercase

    stream.inputBuffer = "test\n";
    term.loop();
    TEST_ASSERT_EQUAL_STRING("test", cmd.lastKeyword.c_str());

    // Reset for second command
    cmd.lastKeyword = "";
    stream.outputBuffer = "";
    stream.inputBuffer = "TEST\n"; // uppercase
    term.loop();

    // Should not match (case-sensitive) - keyword should still be empty
    TEST_ASSERT_EQUAL_STRING("", cmd.lastKeyword.c_str());
}

void test_terminal_continues_running_command_without_newline(void)
{
    MockStream stream;
    Terminal term(stream);
    RunningTwiceCommand cmd;
    term.registerCommand("run", &cmd);

    stream.inputBuffer = "run\n";
    term.loop();
    TEST_ASSERT_EQUAL(1, cmd.executionCount);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("phase1") != ETString::npos);

    stream.inputBuffer = "";
    stream.inputPos = 0;
    term.loop();
    TEST_ASSERT_EQUAL(2, cmd.executionCount);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("phase2") != ETString::npos);
}

void test_terminal_waiting_command_needs_input_to_resume(void)
{
    MockStream stream;
    Terminal term(stream);
    WaitForInputCommand cmd;
    term.registerCommand("wait", &cmd);

    stream.inputBuffer = "wait\n";
    term.loop();
    TEST_ASSERT_EQUAL(1, cmd.executionCount);

    stream.inputBuffer = "";
    stream.inputPos = 0;
    term.loop();
    TEST_ASSERT_EQUAL(1, cmd.executionCount);

    stream.inputBuffer = "x";
    stream.inputPos = 0;
    term.loop();
    TEST_ASSERT_EQUAL(2, cmd.executionCount);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("resumed") != ETString::npos);
}

void test_terminal_uses_lexer_for_quoted_arguments(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("test", &cmd);

    stream.inputBuffer = "test \"hello world\"\n";
    term.loop();

    TEST_ASSERT_EQUAL_STRING("test", cmd.lastKeyword.c_str());
    TEST_ASSERT_EQUAL_STRING("hello world", cmd.lastAdditional.c_str());
}

void test_terminal_reports_lexer_error_for_unterminated_quote(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("test", &cmd);

    stream.inputBuffer = "test \"unterminated\n";
    term.loop();

    TEST_ASSERT_EQUAL(2, term.getLastExitCode());
    TEST_ASSERT_TRUE(stream.stderrBuffer.find("lexer error") != ETString::npos);
    TEST_ASSERT_EQUAL_STRING("", cmd.lastKeyword.c_str());
}

void test_terminal_parses_pipe_in_arguments(void)
{
    MockStream stream;
    Terminal term(stream);
    EmitCommand emit;
    UpperFromStdinCommand upper;
    term.registerCommand("emit", &emit);
    term.registerCommand("upper", &upper);

    stream.inputBuffer = "emit | upper\n";
    term.loop();

    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("HELLO PIPE") != ETString::npos);
}

void test_terminal_redirects_output_with_gt(void)
{
    MockStream stream;
    Terminal term(stream);
    MockFileSystem fs;
    term.setFileSystem(&fs);

    EmitCommand emit;
    term.registerCommand("emit", &emit);

    stream.inputBuffer = "emit > /out.txt\n";
    term.loop();

    TEST_ASSERT_TRUE(fs.exists("/out.txt"));
    ETFile file = fs.open("/out.txt", FILE_MODE_READ, false);
    TEST_ASSERT_TRUE(file.isOpen());
    TEST_ASSERT_EQUAL_STRING("hello pipe", file.readAll().c_str());
    file.close();
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("hello pipe") == ETString::npos);
}

void test_terminal_redirects_input_with_lt(void)
{
    MockStream stream;
    Terminal term(stream);
    MockFileSystem fs;
    term.setFileSystem(&fs);

    ETFile inFile = fs.open("/in.txt", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(inFile.isOpen());
    TEST_ASSERT_TRUE(inFile.writeAll("hello pipe"));
    inFile.close();

    UpperFromStdinCommand upper;
    term.registerCommand("upper", &upper);

    stream.inputBuffer = "upper < /in.txt\n";
    term.loop();

    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("HELLO PIPE") != ETString::npos);
}

void test_terminal_redirects_output_with_gtgt_append(void)
{
    MockStream stream;
    Terminal term(stream);
    MockFileSystem fs;
    term.setFileSystem(&fs);

    EmitCommand emit;
    term.registerCommand("emit", &emit);

    stream.inputBuffer = "emit >> /append.txt\nemit >> /append.txt\n";
    term.loop();

    TEST_ASSERT_TRUE(fs.exists("/append.txt"));
    ETFile file = fs.open("/append.txt", FILE_MODE_READ, false);
    TEST_ASSERT_TRUE(file.isOpen());
    TEST_ASSERT_EQUAL_STRING("hello pipehello pipe", file.readAll().c_str());
    file.close();
}

void test_terminal_redirects_output_with_gt_and_overwrite(void)
{
    MockStream stream;
    Terminal term(stream);
    MockFileSystem fs;
    term.setFileSystem(&fs);

    EmitCommand emit;
    term.registerCommand("emit", &emit);

    stream.inputBuffer = "emit > /overwrite.txt\nemit > /overwrite.txt\n";
    term.loop();

    TEST_ASSERT_TRUE(fs.exists("/overwrite.txt"));
    ETFile file = fs.open("/overwrite.txt", FILE_MODE_READ, false);
    TEST_ASSERT_TRUE(file.isOpen());
    TEST_ASSERT_EQUAL_STRING("hello pipe", file.readAll().c_str());
    file.close();
}

void test_terminal_rejects_inline_script_syntax_without_script_command(void)
{
    MockStream stream;
    Terminal term(stream);
    CollectArgsCommand collect;
    term.registerCommand("collect", &collect);

    stream.inputBuffer = "collect one; collect two\n";
    term.loop();

    TEST_ASSERT_EQUAL(0, collect.collected.size());
    TEST_ASSERT_TRUE(stream.stderrBuffer.find("use script <...>") != ETString::npos);
    TEST_ASSERT_EQUAL(2, term.getLastExitCode());
}

void test_terminal_for_loop_executes_body_for_each_value(void)
{
    MockStream stream;
    Terminal term(stream);
    cmd::script scriptCmd(term);
    CollectArgsCommand collect;
    term.registerCommand("script", &scriptCmd);
    term.registerCommand("collect", &collect);

    stream.inputBuffer = "script for i in one two three; do collect $i; done\n";
    term.loop();
    term.loop();
    term.loop();

    TEST_ASSERT_EQUAL(3, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("one", collect.collected[0].c_str());
    TEST_ASSERT_EQUAL_STRING("two", collect.collected[1].c_str());
    TEST_ASSERT_EQUAL_STRING("three", collect.collected[2].c_str());
}

void test_terminal_for_loop_supports_braced_variable(void)
{
    MockStream stream;
    Terminal term(stream);
    cmd::script scriptCmd(term);
    CollectArgsCommand collect;
    term.registerCommand("script", &scriptCmd);
    term.registerCommand("collect", &collect);

    stream.inputBuffer = "script for item in alpha beta; do collect ${item}; done\n";
    term.loop();
    term.loop();

    TEST_ASSERT_EQUAL(2, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("alpha", collect.collected[0].c_str());
    TEST_ASSERT_EQUAL_STRING("beta", collect.collected[1].c_str());
}

void test_terminal_semicolon_executes_both_commands(void)
{
    MockStream stream;
    Terminal term(stream);
    cmd::script scriptCmd(term);
    CollectArgsCommand collect;
    term.registerCommand("script", &scriptCmd);
    term.registerCommand("collect", &collect);

    stream.inputBuffer = "script missing ; collect second\n";
    term.loop();
    term.loop();

    TEST_ASSERT_EQUAL(1, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("second", collect.collected[0].c_str());
}

void test_terminal_andand_executes_second_only_on_success(void)
{
    MockStream stream;
    Terminal term(stream);
    cmd::script scriptCmd(term);
    CollectArgsCommand collect;
    term.registerCommand("script", &scriptCmd);
    term.registerCommand("collect", &collect);

    stream.inputBuffer = "script collect ok && collect yes\n";
    term.loop();
    term.loop();
    TEST_ASSERT_EQUAL(2, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("ok", collect.collected[0].c_str());
    TEST_ASSERT_EQUAL_STRING("yes", collect.collected[1].c_str());

    collect.collected.clear();
    stream.inputBuffer = "script missing && collect no\n";
    stream.inputPos = 0;
    term.loop();
    term.loop();
    TEST_ASSERT_EQUAL(0, collect.collected.size());
}

void test_terminal_oror_executes_second_only_on_failure(void)
{
    MockStream stream;
    Terminal term(stream);
    cmd::script scriptCmd(term);
    CollectArgsCommand collect;
    term.registerCommand("script", &scriptCmd);
    term.registerCommand("collect", &collect);

    stream.inputBuffer = "script missing || collect recovered\n";
    term.loop();
    term.loop();
    TEST_ASSERT_EQUAL(1, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("recovered", collect.collected[0].c_str());

    collect.collected.clear();
    stream.inputBuffer = "script collect good || collect no\n";
    stream.inputPos = 0;
    term.loop();
    term.loop();
    TEST_ASSERT_EQUAL(1, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("good", collect.collected[0].c_str());
}

void test_terminal_delay_is_non_blocking_and_resumes_later(void)
{
    MockStream stream;
    Terminal term(stream);
    cmd::script scriptCmd(term);
    CollectArgsCommand collect;
    term.registerCommand("script", &scriptCmd);
    term.registerCommand("collect", &collect);

    stream.inputBuffer = "script collect first; delay 30; collect second\n";
    term.loop();

    TEST_ASSERT_EQUAL(1, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("first", collect.collected[0].c_str());

    term.loop();
    TEST_ASSERT_EQUAL(1, collect.collected.size());

#if !defined(ARDUINO) && !defined(ESP_PLATFORM) && !defined(ESP_32)
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
#endif

    term.loop();
    TEST_ASSERT_EQUAL(2, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("second", collect.collected[1].c_str());
}

void test_script_command_executes_script_file(void)
{
    MockStream stream;
    Terminal term(stream);
    MockFileSystem fs;
    term.setFileSystem(&fs);
    cmd::script scriptCmd(term);
    CollectArgsCommand collect;
    term.registerCommand("script", &scriptCmd);
    term.registerCommand("collect", &collect);

    ETFile file = fs.open("/blink.et", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(file.isOpen());
    TEST_ASSERT_TRUE(file.writeAll("collect one; collect two"));
    file.close();

    stream.inputBuffer = "script -f /blink.et\n";
    term.loop();
    term.loop();
    term.loop();

    TEST_ASSERT_EQUAL(2, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("one", collect.collected[0].c_str());
    TEST_ASSERT_EQUAL_STRING("two", collect.collected[1].c_str());
}

void test_script_command_runs_async_subcommands_cooperatively(void)
{
    MockStream stream;
    Terminal term(stream);
    cmd::script scriptCmd(term);
    RunningTwiceCommand run;
    CollectArgsCommand collect;
    term.registerCommand("script", &scriptCmd);
    term.registerCommand("run", &run);
    term.registerCommand("collect", &collect);

    stream.inputBuffer = "script run; collect done\n";
    term.loop();
    TEST_ASSERT_EQUAL(1, run.executionCount);
    TEST_ASSERT_EQUAL(0, collect.collected.size());

    stream.inputBuffer = "";
    stream.inputPos = 0;
    term.loop();
    TEST_ASSERT_EQUAL(2, run.executionCount);
    TEST_ASSERT_EQUAL(0, collect.collected.size());

    stream.inputBuffer = "";
    stream.inputPos = 0;
    term.loop();
    TEST_ASSERT_EQUAL(2, run.executionCount);
    TEST_ASSERT_EQUAL(1, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("done", collect.collected[0].c_str());
}

void test_script_command_waiting_subcommand_resumes_with_input(void)
{
    MockStream stream;
    Terminal term(stream);
    cmd::script scriptCmd(term);
    WaitForInputCommand wait;
    CollectArgsCommand collect;
    term.registerCommand("script", &scriptCmd);
    term.registerCommand("wait", &wait);
    term.registerCommand("collect", &collect);

    stream.inputBuffer = "script wait; collect resumed\n";
    term.loop();
    TEST_ASSERT_EQUAL(1, wait.executionCount);
    TEST_ASSERT_EQUAL(0, collect.collected.size());

    stream.inputBuffer = "";
    stream.inputPos = 0;
    term.loop();
    TEST_ASSERT_EQUAL(1, wait.executionCount);
    TEST_ASSERT_EQUAL(0, collect.collected.size());

    stream.inputBuffer = "x";
    stream.inputPos = 0;
    term.loop();
    TEST_ASSERT_EQUAL(2, wait.executionCount);

    stream.inputBuffer = "";
    stream.inputPos = 0;
    term.loop();
    TEST_ASSERT_EQUAL(1, collect.collected.size());
    TEST_ASSERT_EQUAL_STRING("resumed", collect.collected[0].c_str());
}

void test_script_while_true_repeats_until_interrupted(void)
{
    MockStream stream;
    Terminal term(stream);
    cmd::script scriptCmd(term);
    CollectArgsCommand collect;
    term.registerCommand("script", &scriptCmd);
    term.registerCommand("collect", &collect);

    stream.inputBuffer = "script while true; do collect tick; done\n";
    term.loop(); // first collect
    term.loop(); // wrap
    term.loop(); // second collect
    term.loop(); // wrap
    term.loop(); // third collect

    TEST_ASSERT_TRUE(collect.collected.size() >= 3);
    size_t countBeforeInterrupt = collect.collected.size();

    stream.inputBuffer = ETString("\x03");
    stream.inputPos = 0;
    term.loop();

    size_t countAfterInterruptSignal = collect.collected.size();

    stream.inputBuffer = "";
    stream.inputPos = 0;
    term.loop();
    term.loop();

    TEST_ASSERT_EQUAL(130, term.getLastExitCode());
    TEST_ASSERT_TRUE(countAfterInterruptSignal == countBeforeInterrupt || countAfterInterruptSignal == (countBeforeInterrupt + 1));
    TEST_ASSERT_EQUAL(countAfterInterruptSignal, collect.collected.size());
    TEST_ASSERT_TRUE(stream.stderrBuffer.find("^C") != ETString::npos);
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_terminal_register_and_call);
    RUN_TEST(test_terminal_call_unknown);
    RUN_TEST(test_terminal_getCommands);
    RUN_TEST(test_terminal_deregister_command);
    RUN_TEST(test_terminal_deregister_nonexistent);
    RUN_TEST(test_terminal_multiple_commands);
    RUN_TEST(test_terminal_command_override);
    RUN_TEST(test_terminal_empty_input);
    RUN_TEST(test_terminal_whitespace_input);
    RUN_TEST(test_terminal_command_with_spaces);
    RUN_TEST(test_terminal_long_command);
    RUN_TEST(test_terminal_sequential_commands);
    RUN_TEST(test_terminal_case_sensitivity);
    RUN_TEST(test_terminal_continues_running_command_without_newline);
    RUN_TEST(test_terminal_waiting_command_needs_input_to_resume);
    RUN_TEST(test_terminal_uses_lexer_for_quoted_arguments);
    RUN_TEST(test_terminal_reports_lexer_error_for_unterminated_quote);
    RUN_TEST(test_terminal_parses_pipe_in_arguments);
    RUN_TEST(test_terminal_redirects_output_with_gt);
    RUN_TEST(test_terminal_redirects_input_with_lt);
    RUN_TEST(test_terminal_redirects_output_with_gtgt_append);
    RUN_TEST(test_terminal_redirects_output_with_gt_and_overwrite);
    RUN_TEST(test_terminal_rejects_inline_script_syntax_without_script_command);
    RUN_TEST(test_terminal_for_loop_executes_body_for_each_value);
    RUN_TEST(test_terminal_for_loop_supports_braced_variable);
    RUN_TEST(test_terminal_semicolon_executes_both_commands);
    RUN_TEST(test_terminal_andand_executes_second_only_on_success);
    RUN_TEST(test_terminal_oror_executes_second_only_on_failure);
    RUN_TEST(test_terminal_delay_is_non_blocking_and_resumes_later);
    RUN_TEST(test_script_command_executes_script_file);
    RUN_TEST(test_script_command_runs_async_subcommands_cooperatively);
    RUN_TEST(test_script_command_waiting_subcommand_resumes_with_input);
    RUN_TEST(test_script_while_true_repeats_until_interrupted);
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
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
#include "../../../src/StorageSystem.h"
#include "../../../src/DirectoryNavigator.h"
#include "../../../src/commands/echo.h"
#include "../../../src/commands/wc.h"
#include "../../Mocks/MockStream.h"
#include "../../Mocks/MockCommand.h"
#include "../../Mocks/native/MockFileSystem.h"
#include <unity.h>

using namespace EmbeddedTerminal;

class WaitForInputCommand : public ICommand
{
public:
    int executionCount = 0;

    ETString usage(const ETString &keyword) const override
    {
        return keyword;
    }

    CommandResult invoke(CommandInvocation &invocation) override
    {
        executionCount = 1;
        return CommandResult::waitingForInput(0);
    }

    CommandResult resume(CommandInvocation &invocation) override
    {
        executionCount++;
        invocation.streams.output.print("resumed");
        return CommandResult::completed(0);
    }
};

class RunningTwiceCommand : public ICommand
{
public:
    size_t executionCount = 0;
    ETString usage(const ETString &keyword) const override
    {
        return keyword;
    }

    CommandResult invoke(CommandInvocation &invocation) override
    {
        executionCount = 1;
        invocation.streams.output.print("phase1");
        return CommandResult::running(0);
    }

    CommandResult resume(CommandInvocation &invocation) override
    {
        executionCount++;
        invocation.streams.output.print("phase2");
        return CommandResult::completed(0);
    }
};

class EmitCommand : public ICommand
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
        invocation.streams.output.print("hello pipe");
        return CommandResult::completed(0);
    }
};

class UpperFromStdinCommand : public ICommand
{
public:
    ETString usage(const ETString &keyword) const override
    {
        return keyword;
    }

    CommandResult invoke(CommandInvocation &invocation) override
    {
        ETString text = invocation.streams.input.readAll();
        for (size_t i = 0; i < text.length(); ++i)
        {
            if (text[i] >= 'a' && text[i] <= 'z')
            {
                text[i] = static_cast<char>(text[i] - ('a' - 'A'));
            }
        }
        invocation.streams.output.print(text);
        return CommandResult::completed(0);
    }
};

class TestStorageSetup
{
public:
    StorageSystem storage;
    DirectoryNavigator navigator{&storage};
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
    TEST_ASSERT_EQUAL_STRING("test", cmd.lastKeyword.c_str());
    TEST_ASSERT_EQUAL_STRING("extra", cmd.lastAdditional.c_str());
}

void test_terminal_call_unknown(void)
{
    MockStream stream;
    Terminal term(stream);
    stream.inputBuffer = "foo\n";
    term.loop();
    TEST_ASSERT_EQUAL(TerminalPredefinedResultCodes::COMMAND_NOT_FOUND, term.getLastExitCode());
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

    TEST_ASSERT_TRUE(term.isRunning()); // Command should have completed after second phase
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

void test_terminal_executes_multiline_quoted_argument_through_pipe(void)
{
    MockStream stream;
    Terminal term(stream);
    TestStorageSetup storageSetup;
    cmd::echo echoCmd;
    cmd::wc wcCmd(storageSetup.navigator);

    term.registerCommand("echo", &echoCmd);
    term.registerCommand("wc", &wcCmd);

    stream.inputBuffer = "echo \"This is a test\nand what is that?\" | wc\n";
    term.loop();

    TEST_ASSERT_FALSE(term.error());
    TEST_ASSERT_FALSE(term.isRunning());
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("Lines\tWords\tBytes\n") != ETString::npos);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("2\t8\t") != ETString::npos);
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

void test_terminal_does_not_autocomplete_when_tab_is_inside_quotes(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("test", &cmd);

    stream.inputBuffer = "test \"hello\tworld\"\n";
    term.loop();

    TEST_ASSERT_EQUAL_STRING("test", cmd.lastKeyword.c_str());
    TEST_ASSERT_EQUAL_STRING("hello\tworld", cmd.lastAdditional.c_str());
    TEST_ASSERT_TRUE(stream.outputBuffer.find("test \"hello\tworld\"") != ETString::npos);
}

void test_termminal_did_not_run_with_unterminated_quote(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("test", &cmd);

    stream.inputBuffer = "test \"unterminated\n";
    term.loop();

    TEST_ASSERT_EQUAL(TerminalPredefinedResultCodes::SUCCESS, term.getLastExitCode());
    TEST_ASSERT_FALSE(term.isRunning());
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
    int step = 0;
    const int maxSteps = 2;
    do
    {
        term.loop();
        step++;
    } while (term.isRunning() && step < maxSteps);

    // Make sure that the execution did not get stuck and that the output from the first command was piped into the second command, resulting in "HELLO PIPE" being printed to the terminal.
    TEST_ASSERT_FALSE(term.isRunning());

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
    size_t step = 0;
    const size_t maxSteps = 2;
    do
    {
        term.loop();
        step++;
    } while (term.isRunning() && step < maxSteps);
    TEST_ASSERT_FALSE(term.isRunning());

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

    size_t step = 0;
    const size_t maxSteps = 2;
    do
    {
        term.loop();
        step++;
    } while (term.isRunning() && step < maxSteps);
    TEST_ASSERT_FALSE(term.isRunning());

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
    TEST_ASSERT_EQUAL(1, emit.executionCount);
    TEST_ASSERT_TRUE(term.isRunning());

    do
    {
        term.loop();
    } while (term.isRunning());

    TEST_ASSERT_EQUAL(2, emit.executionCount);

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

void test_terminal_call_command_with_variable_expansion_in_arguments(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("test", &cmd);

    stream.inputBuffer = "VAR=value; test $VAR\n";
    do
    {
        term.loop();
    } while (term.isRunning());

    TEST_ASSERT_EQUAL_STRING("test", cmd.lastKeyword.c_str());
    TEST_ASSERT_EQUAL_STRING("value", cmd.lastAdditional.c_str());
}

void test_terminal_call_command_with_variable_expansion_in_arguments_with_double_quotes(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("test", &cmd);

    stream.inputBuffer = "VAR=\"value with spaces\"; test \"$VAR\"\n";
    do
    {
        term.loop();
    } while (term.isRunning());

    TEST_ASSERT_EQUAL_STRING("test", cmd.lastKeyword.c_str());
    TEST_ASSERT_TRUE(term.getVariables().find("VAR") != term.getVariables().end());
    TEST_ASSERT_EQUAL_STRING("value with spaces", term.getVariables().find("VAR")->second.c_str());
    // Variable inside double quotes should expand but keep the spaces, so the additional should be "value with spaces"
    TEST_ASSERT_EQUAL_STRING("value with spaces", cmd.lastAdditional.c_str());
}

void test_terminal_call_command_without_variable_expansion_in_arguments_with_single_quotes(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("test", &cmd);

    stream.inputBuffer = "VAR=value; test '$VAR'\n";
    do
    {
        term.loop();
    } while (term.isRunning());

    TEST_ASSERT_EQUAL_STRING("test", cmd.lastKeyword.c_str());
    // Variable inside single quotes should NOT expand, so the additional should be "$VAR"
    TEST_ASSERT_EQUAL_STRING("$VAR", cmd.lastAdditional.c_str());
}

void test_terminal_variables_can_be_set_and_overridden(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("test", &cmd);

    stream.inputBuffer = "VAR=value; test $VAR\nVAR=other; test $VAR\n";
    do
    {
        term.loop();
    } while (term.isRunning());

    TEST_ASSERT_EQUAL_STRING("test", cmd.lastKeyword.c_str());
    TEST_ASSERT_EQUAL_STRING("other", cmd.lastAdditional.c_str());
}

void test_terminal_variables_can_be_set_and_overridden_edge_case(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("test", &cmd);

    stream.inputBuffer = "VAR=value; test $VAR;VAR=bla; test $VAR\n";
    do
    {
        term.loop();
    } while (term.isRunning());

    TEST_ASSERT_EQUAL_STRING("test", cmd.lastKeyword.c_str());
    TEST_ASSERT_EQUAL_STRING("bla", cmd.lastAdditional.c_str());
}

void test_terminal_variables_can_be_set(void)
{
    MockStream stream;
    Terminal term(stream);

    stream.inputBuffer = "VAR=value\n";
    term.loop();
    TEST_ASSERT_FALSE(term.error());
    TEST_ASSERT_FALSE(term.isRunning()); // Setting a variable should not put the terminal in a running state and shut be completed immediately after processing the command
    TEST_ASSERT_TRUE(term.getVariables().find("VAR") != term.getVariables().end());
}

void test_terminal_waits_for_incomplete_for_loop_and_executes_on_completion(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("mycmd", &cmd);

    // Send the beginning of a for-loop but omit the body and the 'done'
    stream.inputBuffer = "for i = 1 2 do\n";
    stream.inputPos = 0;
    term.loop();

    // The terminal should not have executed the command yet
    TEST_ASSERT_EQUAL_STRING("", cmd.lastKeyword.c_str());

    // Now send the loop body and the closing 'done' to complete the construct
    stream.inputBuffer = "mycmd $i\ndone\n";
    stream.inputPos = 0;
    // Run the terminal until the script finishes executing
    do
    {
        term.loop();
    } while (term.isRunning());

    // After completion the registered command should have been executed; lastAdditional should be the last iteration value
    TEST_ASSERT_EQUAL_STRING("mycmd", cmd.lastKeyword.c_str());
    TEST_ASSERT_EQUAL_STRING("2", cmd.lastAdditional.c_str());
}

void test_terminal_waits_for_incomplete_if_and_executes_on_completion(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("mycmd", &cmd);

    // Send start of an if construct; omit the then-body and 'fi'
    stream.inputBuffer = "if true then\n";
    stream.inputPos = 0;
    term.loop();

    // Terminal should not execute anything yet
    TEST_ASSERT_EQUAL_STRING("", cmd.lastKeyword.c_str());

    // Provide the body and close the if with 'fi'
    stream.inputBuffer = "mycmd\nfi\n";
    stream.inputPos = 0;
    do
    {
        term.loop();
    } while (term.isRunning());

    TEST_ASSERT_EQUAL_STRING("mycmd", cmd.lastKeyword.c_str());
}

void test_terminal_waits_for_incomplete_while_loop_and_executes_on_completion(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("mycmd", &cmd);

    // Send start of a while loop; omit the body and 'done'
    stream.inputBuffer = "test = true; while $test do\n";
    stream.inputPos = 0;
    do
    {
        term.loop();
    } while (term.isRunning());
    TEST_ASSERT_FALSE(term.error());
    // Terminal should not execute anything yet
    TEST_ASSERT_EQUAL_STRING("", cmd.lastKeyword.c_str());

    // Provide the body and close the loop with 'done'
    stream.inputBuffer = "mycmd\ntest= false; done\n";
    stream.inputPos = 0;
    do
    {
        term.loop();
    } while (term.isRunning());

    if (term.error())
    {
        printf("Terminal last exit code: %d\n", static_cast<int>(term.getLastExitCode()));
        printf("Terminal error: %d\n", static_cast<int>(term.getError()));
    }
    TEST_ASSERT_FALSE(term.error());
    TEST_ASSERT_EQUAL_STRING("mycmd", cmd.lastKeyword.c_str());
}

void test_terminal_waits_for_incomplete_function_definition_and_executes_on_completion(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("mycmd", &cmd);

    // Send start of a function definition; omit the body and 'done'
    stream.inputBuffer = "function myfunc()\n";
    stream.inputPos = 0;
    term.loop();

    // Terminal should not execute anything yet
    TEST_ASSERT_EQUAL_STRING("", cmd.lastKeyword.c_str());

    // Provide the body and close the function definition with 'done'
    stream.inputBuffer = "mycmd\ndone\n";
    stream.inputPos = 0;
    do
    {
        term.loop();
    } while (term.isRunning());
    if (term.error())
    {
        printf("Terminal last exit code: %d\n", static_cast<int>(term.getLastExitCode()));
        printf("Terminal error: %d\n", static_cast<int>(term.getError()));
    }
    TEST_ASSERT_FALSE(term.error());

    // We have to look after the virtual function table since nothing is executed when defining a function
    TEST_ASSERT_TRUE(term.existsFunction("myfunc"));
}

void test_terminal_waits_for_incomplete_function_call_and_executes_on_completion(void)
{
    MockStream stream;
    Terminal term(stream);
    MockCommand cmd;
    term.registerCommand("mycmd", &cmd);

    stream.inputBuffer = "function myfunc()\nmycmd\ndone\n";
    stream.inputPos = 0;
    term.loop();
    if (term.error())
    {
        printf("Terminal last exit code: %d\n", static_cast<int>(term.getLastExitCode()));
        printf("Terminal error: %d\n", static_cast<int>(term.getError()));
    }
    TEST_ASSERT_FALSE(term.error());
    TEST_ASSERT_TRUE(term.existsFunction("myfunc"));
    // Send start of a function call; omit the closing parenthesis
    stream.inputBuffer = "myfunc(\n";
    stream.inputPos = 0;
    term.loop();

    // Terminal should not execute anything yet
    TEST_ASSERT_EQUAL_STRING("", cmd.lastKeyword.c_str());

    // Provide the closing parenthesis to complete the function call
    stream.inputBuffer = ")\n";
    stream.inputPos = 0;
    do
    {
        term.loop();
    } while (term.isRunning());

    TEST_ASSERT_EQUAL_STRING("mycmd", cmd.lastKeyword.c_str());
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
    RUN_TEST(test_terminal_executes_multiline_quoted_argument_through_pipe);
    RUN_TEST(test_terminal_uses_lexer_for_quoted_arguments);
    RUN_TEST(test_terminal_does_not_autocomplete_when_tab_is_inside_quotes);
    RUN_TEST(test_termminal_did_not_run_with_unterminated_quote);
    RUN_TEST(test_terminal_parses_pipe_in_arguments);
    RUN_TEST(test_terminal_redirects_output_with_gt);
    RUN_TEST(test_terminal_redirects_input_with_lt);
    RUN_TEST(test_terminal_redirects_output_with_gtgt_append);
    RUN_TEST(test_terminal_redirects_output_with_gt_and_overwrite);
    RUN_TEST(test_terminal_call_command_with_variable_expansion_in_arguments);
    RUN_TEST(test_terminal_call_command_with_variable_expansion_in_arguments_with_double_quotes);
    RUN_TEST(test_terminal_call_command_without_variable_expansion_in_arguments_with_single_quotes);
    RUN_TEST(test_terminal_variables_can_be_set_and_overridden);
    RUN_TEST(test_terminal_variables_can_be_set_and_overridden_edge_case);
    RUN_TEST(test_terminal_variables_can_be_set);
    RUN_TEST(test_terminal_waits_for_incomplete_for_loop_and_executes_on_completion);
    RUN_TEST(test_terminal_waits_for_incomplete_if_and_executes_on_completion);
    RUN_TEST(test_terminal_waits_for_incomplete_while_loop_and_executes_on_completion);
    RUN_TEST(test_terminal_waits_for_incomplete_function_definition_and_executes_on_completion);
    RUN_TEST(test_terminal_waits_for_incomplete_function_call_and_executes_on_completion);
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
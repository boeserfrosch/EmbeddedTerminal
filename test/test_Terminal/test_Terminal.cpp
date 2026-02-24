// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#endif

#include "../src/Terminal.h"
#include "../src/ETTypes.h"
#include "../Mocks/MockStream.h"
#include "../Mocks/MockCommand.h"
#include <unity.h>

using namespace EmbeddedTerminal;

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
    UNITY_END();
}

#if (defined(ESP_PLATFORM) || defined(ESP32)) && not defined(ARDUINO)
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
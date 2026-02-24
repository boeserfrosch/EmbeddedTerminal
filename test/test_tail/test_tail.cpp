// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "commands/tail.h"
#include "../Mocks/MockFileSystem.h"
#include "../Mocks/CommandRuntimeTestUtils.h"
#include "DirectoryNavigator.h"
#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

using namespace EmbeddedTerminal;
MockFileSystem FS;
DirectoryNavigator dir(&FS);
class TestTail : public cmd::tail
{

public:
    TestTail(MockFileSystem &fs) : cmd::tail(dir)
    {
        FS = fs;
    }

    TestTail() : cmd::tail(dir)
    {
        FS.createFile("file.txt", "hello1234", 9);
        FS.createFile("big.txt", "ABC", 2000);
    }

    const char *getSessionKeyState()
    {
        return SESSION_KEY_STATE;
    }

    const char *getSessionKeyPos()
    {
        return SESSION_KEY_POS;
    }

    const char *getSessionKeyLinesToFind()
    {
        return SESSION_KEY_LINES_TO_FIND;
    }

    const char *getSessionKeyLinesFound()
    {
        return SESSION_KEY_LINES_FOUND;
    }
};

void test_tail_trigger_small_file(void)
{
    TestTail tail;
    ETString keyword = "tail";
    ETString arg = "file.txt";
    ETString result = tail.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("hello1234") != ETString::npos);
}

void test_tail_trigger_large_file(void)
{
    TestTail tail;
    ETString keyword = "tail";
    ETString arg = "big.txt";
    ETString result = tail.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("... File truncated ...") != ETString::npos);
}

void test_tail_trigger_file_not_exists(void)
{
    TestTail tail;
    ETString keyword = "tail";
    ETString arg = "nofile.txt";
    ETString result = tail.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("did not exist!") != ETString::npos);
}

void test_tail_usage(void)
{
    TestTail tail;
    ETString keyword = "tail";
    ETString result = tail.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Returns the last") != ETString::npos);
}

void test_tail_trigger_edge_cases(void)
{
    TestTail tail;
    ETString keyword = "tail";
    ETString arg = "   ";
    ETString result = tail.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("Expected parameter") != ETString::npos);
}

void test_tail_auto_completion_file_suggestions(void)
{
    TestTail tail;
    ETVector<ETString> suggestions = tail.getSuggestions("fi");
    TEST_ASSERT_TRUE(suggestions.size() > 0);
    TEST_ASSERT_TRUE(suggestions[0].contains("file"));
}

void test_tail_auto_completion_another_file(void)
{
    TestTail tail;
    ETVector<ETString> suggestions = tail.getSuggestions("bi");
    TEST_ASSERT_TRUE(suggestions.size() > 0);
    TEST_ASSERT_TRUE(suggestions[0].contains("big"));
}

void test_tail_streaming_execution(void)
{
    MockFileSystem fs;
    DirectoryNavigator dir(&fs);
    fs.createFile("/file.txt", "line1\nline2\nline3\nline4\nline5\nline6\nline7\nline8\nline9\nline10\nline11\nline12\n");

    EmptyInputChannel stdinChannel;
    MockStream stream;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    TestTail tail(fs);

    CommandInvocation invocation({"tail", "file.txt", context, stdinChannel, stdoutChannel, stderrChannel});
    tail.execute(invocation);
    TEST_ASSERT_MESSAGE(invocation.context.variables.find(tail.getSessionKeyState()) != invocation.context.variables.end(), "Expected session state variable to be set");
    TEST_ASSERT_MESSAGE(invocation.context.variables[tail.getSessionKeyState()] == "FindingStartPosition", "Expected initial state to be FindingStartPosition");
    TEST_ASSERT_MESSAGE(invocation.context.variables.find(tail.getSessionKeyPos()) != invocation.context.variables.end(), "Expected session position variable to be set");
    TEST_ASSERT_MESSAGE(invocation.context.variables[tail.getSessionKeyPos()] == "0", "Expected initial file end offset position to be at end of file");
    TEST_ASSERT_MESSAGE(invocation.context.variables.find(tail.getSessionKeyLinesToFind()) != invocation.context.variables.end(), "Expected lines to find variable to be set");
    TEST_ASSERT_MESSAGE(invocation.context.variables[tail.getSessionKeyLinesToFind()] == "10", "Expected lines to find to be 10");
    TEST_ASSERT_MESSAGE(invocation.context.variables.find(tail.getSessionKeyLinesFound()) != invocation.context.variables.end(), "Expected lines found variable to be set");
    TEST_ASSERT_MESSAGE(invocation.context.variables[tail.getSessionKeyLinesFound()] == "0", "Expected lines found to be 0");

    auto result = tail.execute(invocation);
    TEST_ASSERT_MESSAGE(invocation.context.variables[tail.getSessionKeyState()] == "Streaming", "Expected state to be Streaming after second invocation");
    TEST_ASSERT_MESSAGE(result.exitCode == 0, "Expected exit code to be 0");

    auto result2 = tail.execute(invocation);

    TEST_ASSERT_MESSAGE(result2.state == CommandExecutionState::Completed, "Expected command to be completed after streaming");
    TEST_ASSERT_MESSAGE(result2.exitCode == 0, "Expected exit code to be 0 after streaming");

    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line11") != ETString::npos, "Expected to find line11 in output");
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line2") == ETString::npos, "Expected not to find line2 in output");
}

void test_tail_streaming_execution_file_not_exists(void)
{
    MockFileSystem fs;
    DirectoryNavigator dir(&fs);

    EmptyInputChannel stdinChannel;
    MockStream stream;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    TestTail tail(fs);

    CommandInvocation invocation({"tail", "nofile.txt", context, stdinChannel, stdoutChannel, stderrChannel});
    auto result = tail.execute(invocation);
    TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Completed, "Expected command to complete even if file does not exist");
    TEST_ASSERT_MESSAGE(result.exitCode != 0, "Expected non-zero exit code when file does not exist");
    TEST_ASSERT_MESSAGE(stream.stderrBuffer.find("did not exist") != ETString::npos, "Expected error message about file not existing");
}

void test_tail_streaming_execution_edge_cases(void)
{
    MockFileSystem fs;
    DirectoryNavigator dir(&fs);
    fs.createFile("/file.txt", "line1\nline2\nline3\nline4\nline5\nline6\nline7\nline8\nline9\nline10\nline11\nline12\n");

    EmptyInputChannel stdinChannel;
    MockStream stream;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    TestTail tail(fs);

    CommandInvocation invocation({"tail", "file.txt", context, stdinChannel, stdoutChannel, stderrChannel});
    tail.execute(invocation);

    // Simulate file growing between invocations
    auto file = dir.open("/file.txt", "w", false);
    file.writeAll("new line13\nnew line14\n");
    file.close();

    auto result = tail.execute(invocation);
    TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Running, "Expected command to still be running after file size change");

    auto result2 = tail.execute(invocation);
    TEST_ASSERT_MESSAGE(result2.state == CommandExecutionState::Completed, "Expected command to complete even if file size changed");
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line11") != ETString::npos, "Expected to find line11 in output");
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("new line13") != ETString::npos, "Expected to find new line13 in output");
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line2") == ETString::npos, "Expected not to find line2 in output");
}

void test_tail_streaming_execution_multiple_invocations(void)
{
    MockFileSystem fs;
    DirectoryNavigator dir(&fs);
    fs.createFile("/file.txt", "line1\nline2\nline3\nline4\nline5\nline6\nline7\nline8\nline9\nline10\nline11\nline12\n");

    EmptyInputChannel stdinChannel;
    MockStream stream;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    TestTail tail(fs);

    CommandInvocation invocation({"tail", "file.txt", context, stdinChannel, stdoutChannel, stderrChannel});
    tail.execute(invocation);

    auto result = tail.execute(invocation);
    TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Running, "Expected command to be running after first streaming execution");

    auto result2 = tail.execute(invocation);
    TEST_ASSERT_MESSAGE(result2.state == CommandExecutionState::Completed, "Expected command to complete after second streaming execution");
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line11") != ETString::npos, "Expected to find line11 in output");
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line2") == ETString::npos, "Expected not to find line2 in output");
}

void test_tail_lines_option(void)
{
    MockFileSystem fs;
    DirectoryNavigator dir(&fs);
    fs.createFile("/file.txt", "line1\nline2\nline3\nline4\nline5\nline6\nline7\nline8\nline9\nline10\nline11\nline12");

    EmptyInputChannel stdinChannel;
    MockStream stream;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    TestTail tail(fs);

    CommandInvocation invocation({"tail", "-n 5 file.txt", context, stdinChannel, stdoutChannel, stderrChannel});

    auto result = tail.execute(invocation);
    TEST_ASSERT_MESSAGE(result.exitCode == 0, "Expected exit code to be 0 after execution with -n option");
    TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Running, "Expected command to be running after first execution with -n option");
    TEST_ASSERT_MESSAGE(invocation.context.variables.find(tail.getSessionKeyState()) != invocation.context.variables.end(), "Expected session state variable to be set");
    TEST_ASSERT_MESSAGE(invocation.context.variables[tail.getSessionKeyState()] == "FindingStartPosition", "Expected initial state to be FindingStartPosition");
    TEST_ASSERT_MESSAGE(invocation.context.variables.find(tail.getSessionKeyPos()) != invocation.context.variables.end(), "Expected session position variable to be set");
    TEST_ASSERT_MESSAGE(invocation.context.variables[tail.getSessionKeyPos()] == "0", "Expected initial file end offset position to be at end of file");
    TEST_ASSERT_MESSAGE(invocation.context.variables.find(tail.getSessionKeyLinesToFind()) != invocation.context.variables.end(), "Expected lines to find variable to be set");
    TEST_ASSERT_MESSAGE(invocation.context.variables[tail.getSessionKeyLinesToFind()] == "5", "Expected lines to find to be 5");
    TEST_ASSERT_MESSAGE(invocation.context.variables.find(tail.getSessionKeyLinesFound()) != invocation.context.variables.end(), "Expected lines found variable to be set");
    TEST_ASSERT_MESSAGE(invocation.context.variables[tail.getSessionKeyLinesFound()] == "0", "Expected lines found to be 0");

    auto result2 = tail.execute(invocation);
    TEST_ASSERT_MESSAGE(invocation.context.variables[tail.getSessionKeyState()] == "Streaming", "Expected state to be Streaming after execution");
    TEST_ASSERT_MESSAGE(result2.exitCode == 0, "Expected exit code to be 0 after execution");

    auto result3 = tail.execute(invocation);

    TEST_ASSERT_MESSAGE(result3.state == CommandExecutionState::Completed, "Expected command to complete when using -n option");
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line8") != ETString::npos, "Expected to find line8 in output");
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line7") == ETString::npos, "Expected not to find line7 in output");
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_tail_trigger_small_file);
    RUN_TEST(test_tail_trigger_large_file);
    RUN_TEST(test_tail_trigger_file_not_exists);
    RUN_TEST(test_tail_usage);
    RUN_TEST(test_tail_trigger_edge_cases);
    RUN_TEST(test_tail_auto_completion_file_suggestions);
    RUN_TEST(test_tail_auto_completion_another_file);
    RUN_TEST(test_tail_streaming_execution);
    RUN_TEST(test_tail_streaming_execution_file_not_exists);
    RUN_TEST(test_tail_streaming_execution_edge_cases);
    RUN_TEST(test_tail_streaming_execution_multiple_invocations);
    RUN_TEST(test_tail_lines_option);
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
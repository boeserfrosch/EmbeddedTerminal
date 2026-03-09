// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "commands/tail.h"
#include "../Mocks/native/MockFileSystem.h"
#include "../Mocks/CommandRuntimeTestUtils.h"
#include "DirectoryNavigator.h"
#include <unity.h>
#include "StorageSystem.h"
#include "../Mocks/native/MockStorageMedia.h"

IStorageSystem *storage = nullptr;
DirectoryNavigator *dir = nullptr;

void setUp(void)
{
    storage = new StorageSystem();
    auto media = new MockStorageMedia("mock", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storage->mountMedia(media, "");
    dir = new DirectoryNavigator(storage);
    storage->open("file.txt", "w", true).writeAll("hello1234");
    auto f = storage->open("big.txt", "w", true);
    for (int i = 0; i < 1000; i++)
    {
        f.writeAll("ABC");
    }
}
void tearDown(void)
{
    auto medias = storage->media();
    for (auto media : medias)
    {
        storage->unmountMedia(media->name());
        delete media;
    }
    delete dir;
    dir = nullptr;
    delete storage;
    storage = nullptr;
}

using namespace EmbeddedTerminal;

void test_tail_trigger_small_file(void)
{
    cmd::tail tail(*dir);
    ETString keyword = "tail";
    ETString arg = "file.txt";
    ETString result = tail.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("hello1234") != ETString::npos);
}

void test_tail_trigger_large_file(void)
{
    cmd::tail tail(*dir);
    ETString keyword = "tail";
    ETString arg = "big.txt";
    ETString result = tail.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("... File truncated ...") != ETString::npos);
}

void test_tail_trigger_file_not_exists(void)
{
    cmd::tail tail(*dir);
    ETString keyword = "tail";
    ETString arg = "nofile.txt";
    ETString result = tail.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("did not exist!") != ETString::npos);
}

void test_tail_usage(void)
{
    cmd::tail tail(*dir);
    ETString keyword = "tail";
    ETString result = tail.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Returns the last") != ETString::npos);
}

void test_tail_trigger_edge_cases(void)
{
    cmd::tail tail(*dir);
    ETString keyword = "tail";
    ETString arg = "   ";
    ETString result = tail.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("Expected parameter") != ETString::npos);
}

void test_tail_auto_completion_file_suggestions(void)
{
    cmd::tail tail(*dir);
    ETVector<ETString> suggestions = tail.getSuggestions("fi");
    TEST_ASSERT_TRUE(suggestions.size() > 0);
    TEST_ASSERT_TRUE(suggestions[0].contains("file"));
}

void test_tail_auto_completion_another_file(void)
{
    cmd::tail tail(*dir);
    ETVector<ETString> suggestions = tail.getSuggestions("bi");
    TEST_ASSERT_TRUE(suggestions.size() > 0);
    TEST_ASSERT_TRUE(suggestions[0].contains("big"));
}

void test_tail_streaming_execution(void)
{
    storage->open("/file.txt", "w", true).writeAll("line1\nline2\nline3\nline4\nline5\nline6\nline7\nline8\nline9\nline10\nline11\nline12\n");

    EmptyInputChannel stdinChannel;
    MockStream stream;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    cmd::tail tail(*dir);

    CommandInvocation invocation({"tail", "file.txt", context, stdinChannel, stdoutChannel, stderrChannel});
    tail.execute(invocation);

    auto result = tail.execute(invocation);
    TEST_ASSERT_MESSAGE(result.exitCode == 0, "Expected exit code to be 0");

    auto result2 = tail.execute(invocation);

    TEST_ASSERT_MESSAGE(result2.state == CommandExecutionState::Completed, "Expected command to be completed after streaming");
    TEST_ASSERT_MESSAGE(result2.exitCode == 0, "Expected exit code to be 0 after streaming");

    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line11") != ETString::npos, "Expected to find line11 in output");
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line2") == ETString::npos, "Expected not to find line2 in output");
}

void test_tail_streaming_execution_file_not_exists(void)
{
    EmptyInputChannel stdinChannel;
    MockStream stream;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    cmd::tail tail(*dir);

    CommandInvocation invocation({"tail", "nofile.txt", context, stdinChannel, stdoutChannel, stderrChannel});
    auto result = tail.execute(invocation);
    TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Completed, "Expected command to complete even if file does not exist");
    TEST_ASSERT_MESSAGE(result.exitCode != 0, "Expected non-zero exit code when file does not exist");
    TEST_ASSERT_MESSAGE(stream.stderrBuffer.find("did not exist") != ETString::npos, "Expected error message about file not existing");
}

void test_tail_streaming_execution_edge_cases(void)
{
    storage->open("/file.txt", "w", true).writeAll("line1\nline2\nline3\nline4\nline5\nline6\nline7\nline8\nline9\nline10\nline11\nline12\n");

    EmptyInputChannel stdinChannel;
    MockStream stream;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    cmd::tail tail(*dir);

    CommandInvocation invocation({"tail", "file.txt", context, stdinChannel, stdoutChannel, stderrChannel});
    tail.execute(invocation);

    // Simulate file growing between invocations
    auto file = storage->open("/file.txt", "w", false);
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
    storage->open("/file.txt", "w", true).writeAll("line1\nline2\nline3\nline4\nline5\nline6\nline7\nline8\nline9\nline10\nline11\nline12\n");

    EmptyInputChannel stdinChannel;
    MockStream stream;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    cmd::tail tail(*dir);

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
    storage->open("/file.txt", "w", true).writeAll("line1\nline2\nline3\nline4\nline5\nline6\nline7\nline8\nline9\nline10\nline11\nline12");

    EmptyInputChannel stdinChannel;
    MockStream stream;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    cmd::tail tail(*dir);

    CommandInvocation invocation({"tail", "-n 5 file.txt", context, stdinChannel, stdoutChannel, stderrChannel});

    auto result = tail.execute(invocation);
    TEST_ASSERT_MESSAGE(result.exitCode == 0, "Expected exit code to be 0 after execution with -n option");
    TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Running, "Expected command to be running after first execution with -n option");

    auto result2 = tail.execute(invocation);
    TEST_ASSERT_MESSAGE(result2.exitCode == 0, "Expected exit code to be 0 after execution");

    auto result3 = tail.execute(invocation);

    TEST_ASSERT_MESSAGE(result3.state == CommandExecutionState::Completed, "Expected command to complete when using -n option");
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line8") != ETString::npos, "Expected to find line8 in output");
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line7") == ETString::npos, "Expected not to find line7 in output");
}

void test_tail_lines_long_option(void)
{
    storage->open("/file.txt", "w", true).writeAll("line1\nline2\nline3\nline4\nline5\nline6\n");

    EmptyInputChannel stdinChannel;
    MockStream stream;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    cmd::tail tail(*dir);
    CommandInvocation invocation({"tail", "--lines 3 file.txt", context, stdinChannel, stdoutChannel, stderrChannel});

    tail.execute(invocation);
    tail.execute(invocation);
    auto result = tail.execute(invocation);

    TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Completed, "Expected command to complete when using --lines option");
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line6") != ETString::npos, "Expected to find last lines in output");
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line1") == ETString::npos, "Expected not to find first line in output");
}

void test_tail_invalid_line_count_option(void)
{
    EmptyInputChannel stdinChannel;
    MockStream stream;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    cmd::tail tail(*dir);
    CommandInvocation invocation({"tail", "-n 0 file.txt", context, stdinChannel, stdoutChannel, stderrChannel});
    auto result = tail.execute(invocation);

    TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Completed, "Expected invalid line count to complete with error");
    TEST_ASSERT_TRUE(result.exitCode != 0);
    TEST_ASSERT_MESSAGE(stream.stderrBuffer.find("Invalid number of lines") != ETString::npos, "Expected invalid line-count error");
}

void test_tail_invalid_option_returns_error(void)
{
    EmptyInputChannel stdinChannel;
    MockStream stream;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    cmd::tail tail(*dir);
    CommandInvocation invocation({"tail", "-z file.txt", context, stdinChannel, stdoutChannel, stderrChannel});
    auto result = tail.execute(invocation);

    TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Completed, "Expected invalid option to complete with error");
    TEST_ASSERT_TRUE(result.exitCode != 0);
    TEST_ASSERT_MESSAGE(!stream.stderrBuffer.empty(), "Expected error output for invalid option path");
}

void test_tail_empty_file_streaming_returns_read_error(void)
{
    storage->open("/empty.txt", "w", true).writeAll("");

    EmptyInputChannel stdinChannel;
    MockStream stream;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    cmd::tail tail(*dir);
    CommandInvocation invocation({"tail", "empty.txt", context, stdinChannel, stdoutChannel, stderrChannel});

    tail.execute(invocation);
    tail.execute(invocation);
    auto result = tail.execute(invocation);

    TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Completed, "Expected empty file tail to fail and complete");
    TEST_ASSERT_EQUAL(EmbeddedTerminal::cmd::errorCodes::TAIL_CMD_ERROR_FAILED_TO_READ, result.exitCode);
    TEST_ASSERT_MESSAGE(stream.stderrBuffer.find("Failed to read from file") != ETString::npos, "Expected failed-read error message");
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
    RUN_TEST(test_tail_lines_long_option);
    RUN_TEST(test_tail_invalid_line_count_option);
    RUN_TEST(test_tail_invalid_option_returns_error);
    RUN_TEST(test_tail_empty_file_streaming_returns_read_error);
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
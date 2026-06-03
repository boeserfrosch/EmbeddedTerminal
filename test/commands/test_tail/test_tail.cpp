// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "commands/tail.h"
#include "../../Mocks/native/MockFileSystem.h"
#include "../utils.h"
#include "DirectoryNavigator.h"
#include <unity.h>
#include "StorageSystem.h"
#include "../../Mocks/native/MockStorageMedia.h"
#include <memory>

IStorageSystem *storage = nullptr;
DirectoryNavigator *dir = nullptr;
static std::shared_ptr<MockStorageMedia> media;

void setUp(void)
{
    storage = new StorageSystem();
    media = std::make_shared<MockStorageMedia>("mock", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storage->mountMedia(media, "");
    dir = new DirectoryNavigator(storage);
    auto file = storage->open("/file.txt", "w", true);
    file.writeAll("hello1234");
    file.close();
    auto f = storage->open("big.txt", "w", true);
    for (int i = 0; i < 1000; i++)
    {
        f.writeAll("ABC");
    }
    f.close();
}
void tearDown(void)
{
    if (media)
    {
        bool unmountResult = storage->unmountMedia(media->name());
        if (!unmountResult)
        {
            TEST_FAIL_MESSAGE(("Failed to unmount media: " + std::string(media->name())).c_str());
        }
        media.reset();
    }
    delete dir;
    dir = nullptr;
    delete storage;
    storage = nullptr;
}

using namespace EmbeddedTerminal;

void test_tail_trigger_file_not_exists(void)
{
    cmd::tail tail(*dir);
    ETString keyword = "tail";
    ETString arg = "nofile.txt";
    TestCommandInvocationHandle iHandle("tail", {arg});
    ;
    CommandResult result = tail.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.error.contains("did not exist"));
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
    TestCommandInvocationHandle iHandle("tail");
    ;
    CommandResult result = tail.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.error.contains("Invalid options"));
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
    auto file = storage->open("/file.txt", "w", true);
    file.writeAll("line1\nline2\nline3\nline4\nline5\nline6\nline7\nline8\nline9\nline10\nline11\nline12\n");
    file.close();

    cmd::tail tail(*dir);
    TestCommandInvocationHandle iHandle("tail", {"file.txt"});
    ;
    auto result = tail.invoke(iHandle.invocation);
    TEST_ASSERT_MESSAGE(result.exitCode == 0, "Expected exit code to be 0");

    auto result2 = tail.invoke(iHandle.invocation);

    TEST_ASSERT_MESSAGE(result2.state == CommandExecutionState::Completed, "Expected command to be completed after streaming");
    switch (result2.exitCode)
    {
    case cmd::tail::ErrorCode::NONE:
        break;
    case cmd::tail::ErrorCode::INVALID_OPTIONS:
        TEST_FAIL_MESSAGE("Expected no error, but got INVALID_OPTIONS");
        break;
    case cmd::tail::ErrorCode::INVALID_NUMBER_OF_LINES:
        TEST_FAIL_MESSAGE("Expected no error, but got INVALID_NUMBER_OF_LINES");
        break;
    case cmd::tail::ErrorCode::FILE_NOT_FOUND:
        TEST_FAIL_MESSAGE("Expected no error, but got FILE_NOT_FOUND");
        break;
    case cmd::tail::ErrorCode::IS_DIRECTORY:
        TEST_FAIL_MESSAGE("Expected no error, but got IS_DIRECTORY");
        break;
    case cmd::tail::ErrorCode::FAILED_TO_OPEN_FILE:
        TEST_FAIL_MESSAGE("Expected no error, but got FAILED_TO_OPEN_FILE");
        break;
    case cmd::tail::ErrorCode::FAILED_TO_SEEK:
        TEST_FAIL_MESSAGE("Expected no error, but got FAILED_TO_SEEK");
        break;
    case cmd::tail::ErrorCode::FAILED_TO_READ:
        TEST_FAIL_MESSAGE("Expected no error, but got FAILED_TO_READ");
        break;
    default:
        TEST_FAIL_MESSAGE(("Expected no error, but got unknown error code: " + toETString(result2.exitCode)).c_str());
    }

    TEST_ASSERT_TRUE_MESSAGE(iHandle.output.contains("line11"), "Expected to find line11 in output");
    TEST_ASSERT_FALSE_MESSAGE(iHandle.output.contains("line2"), "Expected not to find line2 in output");
}

void test_tail_streaming_execution_file_not_exists(void)
{
    EmptyInputChannel input;
    MockStream stream;
    StreamBackedOutputChannel output(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel error(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    cmd::tail tail(*dir);
    ETString keyword = "tail";
    ETVector<ETString> arguments = {"nofile.txt"};
    CommandInvocation invocation({keyword, arguments, context, input, output, error});
    auto result = tail.invoke(invocation);
    TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Completed, "Expected command to complete even if file does not exist");
    TEST_ASSERT_MESSAGE(result.exitCode != 0, "Expected non-zero exit code when file does not exist");
    TEST_ASSERT_MESSAGE(stream.stderrBuffer.find("did not exist") != ETString::npos, "Expected error message about file not existing");
}

void test_tail_streaming_execution_multiple_invocations(void)
{
    auto file = storage->open("/file.txt", "w", true);
    file.writeAll("line1\nline2\nline3\nline4\nline5\nline6\nline7\nline8\nline9\nline10\nline11\nline12\n");
    file.close();

    EmptyInputChannel input;
    MockStream stream;
    StreamBackedOutputChannel output(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel error(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    cmd::tail tail(*dir);
    ETString keyword = "tail";
    ETVector<ETString> arguments = {"file.txt"};
    CommandInvocation invocation({keyword, arguments, context, input, output, error});

    auto result2 = tail.invoke(invocation);
    TEST_ASSERT_MESSAGE(result2.state == CommandExecutionState::Completed, "Expected command to complete after first streaming execution");
    TEST_MESSAGE(("Stdout buffer after first execution: " + stream.stdoutBuffer).c_str());
    TEST_ASSERT_EQUAL(0, result2.exitCode);
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line11") != ETString::npos, "Expected to find line11 in output");
    // Since default of -n is 10, line2 should not be in the output, as it is the 11th line from the end (line1 is the 12th)
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line2") == ETString::npos, "Expected not to find line2 in output");
}

void test_tail_lines_option(void)
{
    auto file = storage->open("/file5.txt", "w", true);
    file.writeAll("line1\nline2\nline3\nline4\nline5\nline6\nline7\nline8\nline9\nline10\nline11\nline12");
    file.close();

    // #####
    EmptyInputChannel input;
    MockStream stream;
    StreamBackedOutputChannel output(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel error(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    cmd::tail tail(*dir);
    ETString keyword = "tail";
    ETVector<ETString> arguments = {"file5.txt", "-n", "5"};
    CommandInvocation invocation({keyword, arguments, context, input, output, error});

    auto result = tail.invoke(invocation);
    TEST_ASSERT_MESSAGE(result.exitCode == 0, "Expected exit code to be 0 after execution with -n option");
    TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Completed, "Expected command to be completed after first execution with -n option with value 5");

    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line8") != ETString::npos, "Expected to find line8 in output");
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line7") == ETString::npos, "Expected not to find line7 in output");
}

void test_tail_lines_long_option(void)
{
    auto file = storage->open("/file.txt", "w", true);
    file.writeAll("line1\nline2\nline3\nline4\nline5\nline6\n");
    file.close();

    EmptyInputChannel input;
    MockStream stream;
    StreamBackedOutputChannel output(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel error(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    cmd::tail tail(*dir);
    ETString keyword = "tail";
    ETVector<ETString> arguments = {"--lines", "3", "file.txt"};
    CommandInvocation invocation({keyword, arguments, context, input, output, error});

    auto result = tail.invoke(invocation);

    TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Completed, "Expected command to complete when using --lines option");
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line6") != ETString::npos, "Expected to find last lines in output");
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("line1") == ETString::npos, "Expected not to find first line in output");
}

void test_tail_invalid_line_count_option(void)
{
    EmptyInputChannel input;
    MockStream stream;
    StreamBackedOutputChannel output(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel error(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    cmd::tail tail(*dir);
    ETString keyword = "tail";
    ETVector<ETString> arguments = {"-n", "0", "file.txt"};
    CommandInvocation invocation({keyword, arguments, context, input, output, error});
    auto result = tail.invoke(invocation);

    TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Completed, "Expected invalid line count to complete with error");
    TEST_ASSERT_TRUE(result.exitCode != 0);
    TEST_ASSERT_MESSAGE(stream.stderrBuffer.find("Invalid number of lines") != ETString::npos, "Expected invalid line-count error");
}

void test_tail_invalid_option_returns_error(void)
{
    EmptyInputChannel input;
    MockStream stream;
    StreamBackedOutputChannel output(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel error(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    cmd::tail tail(*dir);
    ETString keyword = "tail";
    ETVector<ETString> arguments = {"-z", "file.txt"};
    CommandInvocation invocation({keyword, arguments, context, input, output, error});
    auto result = tail.invoke(invocation);

    TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Completed, "Expected invalid option to complete with error");
    TEST_ASSERT_TRUE(result.exitCode != 0);
    TEST_ASSERT_MESSAGE(!stream.stderrBuffer.empty(), "Expected error output for invalid option path");
}

void test_tail_empty_file_streaming_returns_no_error(void)
{
    auto file = storage->open("/empty.txt", "w", true);
    file.writeAll("");
    file.close();

    {
        auto file = storage->open("/empty.txt", "r");
        size_t fileSize = file.size();
        file.close();
        TEST_ASSERT_EQUAL(0, fileSize);
    }

    EmptyInputChannel input;
    MockStream stream;
    StreamBackedOutputChannel output(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel error(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    cmd::tail tail(*dir);
    ETString keyword = "tail";
    ETVector<ETString> arguments = {"empty.txt"};
    CommandInvocation invocation({keyword, arguments, context, input, output, error});

    auto result = tail.invoke(invocation);

    TEST_ASSERT_EQUAL(CommandExecutionState::Completed, result.state);
    TEST_ASSERT_EQUAL(EmbeddedTerminal::cmd::tail::ErrorCode::NONE, result.exitCode);
    TEST_ASSERT_TRUE_MESSAGE(stream.stderrBuffer.empty(), "Expected no error output when streaming empty file");
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_tail_trigger_file_not_exists);
    RUN_TEST(test_tail_usage);
    RUN_TEST(test_tail_trigger_edge_cases);
    RUN_TEST(test_tail_auto_completion_file_suggestions);
    RUN_TEST(test_tail_auto_completion_another_file);
    RUN_TEST(test_tail_streaming_execution);
    RUN_TEST(test_tail_streaming_execution_file_not_exists);
    RUN_TEST(test_tail_streaming_execution_multiple_invocations);
    RUN_TEST(test_tail_lines_option);
    RUN_TEST(test_tail_lines_long_option);
    RUN_TEST(test_tail_invalid_line_count_option);
    RUN_TEST(test_tail_invalid_option_returns_error);
    RUN_TEST(test_tail_empty_file_streaming_returns_no_error);
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
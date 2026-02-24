// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "commands/cat.h"
#include "../src/DirectoryNavigator.h"
#include "../Mocks/MockFileSystem.h"
#include "../Mocks/MockStream.h"
#include "../Mocks/CommandRuntimeTestUtils.h"

#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

void test_cat_trigger_small_file(void)
{
    MockFileSystem fs;
    EmbeddedTerminal::DirectoryNavigator dir(&fs);
    fs.createFile("/file.txt", "hello1234", 9); // Small file
    EmbeddedTerminal::cmd::cat cat(dir);

    ETString keyword = "cat";
    ETString arg = "file.txt";
    ETString result = cat.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("hello1234") != ETString::npos);
}

void test_cat_trigger_large_file(void)
{
    MockFileSystem fs;
    EmbeddedTerminal::DirectoryNavigator dir(&fs);
    fs.createFile("/big.txt", "ABC", 2000); // Fake large file
    EmbeddedTerminal::cmd::cat cat(dir);
    ETString keyword = "cat";
    ETString arg = "big.txt";
    ETString result = cat.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("... File truncated ...") != ETString::npos);
}

void test_cat_trigger_file_not_exists(void)
{
    MockFileSystem fs;
    EmbeddedTerminal::DirectoryNavigator dir(&fs);
    EmbeddedTerminal::cmd::cat cat(dir);

    ETString keyword = "cat";
    ETString arg = "nofile.txt";
    ETString result = cat.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("did not exist!") != ETString::npos);
}

void test_cat_usage(void)
{
    MockFileSystem fs;
    EmbeddedTerminal::DirectoryNavigator dir(&fs);
    EmbeddedTerminal::cmd::cat cat(dir);
    ETString keyword = "cat";
    ETString result = cat.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Returns the content") != ETString::npos);
}

void test_cat_trigger_edge_cases(void)
{
    MockFileSystem fs;
    EmbeddedTerminal::DirectoryNavigator dir(&fs);
    EmbeddedTerminal::cmd::cat cat(dir);
    ETString keyword = "cat";
    ETString arg = "   ";
    ETString result = cat.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("expected") != ETString::npos);
    ETString arg2 = "did_not_exist.txt";
    ETString result2 = cat.trigger(keyword, arg2);
    TEST_ASSERT_TRUE(result2.find("did not exist!") != ETString::npos);
}

void test_cat_execute_small_file_writes_stdout(void)
{
    MockFileSystem fs;
    EmbeddedTerminal::DirectoryNavigator dir(&fs);
    fs.createFile("/file.txt", "hello1234", 9);
    EmbeddedTerminal::cmd::cat cat(dir);
    MockStream stream;

    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    ETString keyword = "cat";
    ETString arg = "file.txt";

    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{keyword, arg, context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = cat.execute(invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("hello1234") != ETString::npos);
    TEST_ASSERT_TRUE(stream.stderrBuffer.empty());
}

void test_cat_execute_missing_file_writes_stderr(void)
{
    MockFileSystem fs;
    EmbeddedTerminal::DirectoryNavigator dir(&fs);
    EmbeddedTerminal::cmd::cat cat(dir);
    MockStream stream;

    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    ETString keyword = "cat";
    ETString arg = "missing.txt";

    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{keyword, arg, context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = cat.execute(invocation);

    TEST_ASSERT_EQUAL(2, result.exitCode);
    TEST_ASSERT_TRUE(stream.stderrBuffer.find("did not exist") != ETString::npos);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.empty());
}

void test_cat_manual_stream_debug(void)
{
    MockFileSystem fs;
    EmbeddedTerminal::DirectoryNavigator dir(&fs);
    ETString largeContent = "";
    for (int i = 0; i < 30; i++)
    {
        largeContent += "0123456789";
    }
    largeContent += "Extra";
    fs.createFile("/large.txt", largeContent, 0);

    // Manually read the file Chunk by chunk
    auto file1 = dir.open("large.txt", "r", false);
    TEST_ASSERT_TRUE(file1.isOpen());

    unsigned char buf1[256];
    size_t read1 = file1.read(buf1, 256);
    TEST_ASSERT_EQUAL(256u, read1);
    file1.close();

    // Second read should start from beginning
    auto file2 = dir.open("large.txt", "r", false);
    TEST_ASSERT_TRUE(file2.isOpen());
    file2.seek(256); // Seek to where first read ended

    unsigned char buf2[256];
    size_t read2 = file2.read(buf2, 256);
    TEST_ASSERT_EQUAL(49u, read2); // Should read remaining 49 bytes
    file2.close();
}

void test_cat_execute_streaming_simple(void)
{
    MockFileSystem fs;
    EmbeddedTerminal::DirectoryNavigator dir(&fs);
    ETString largeContent = "";
    for (int i = 0; i < 60; i++)
    {
        largeContent += "0123456789";
    }
    largeContent += "Extra";
    fs.createFile("/large.txt", largeContent, 0);

    EmbeddedTerminal::cmd::cat cat(dir);
    MockStream stream;

    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    // First invocation
    CommandInvocation invocation1{"cat", "large.txt", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result1 = cat.execute(invocation1);

    TEST_ASSERT_EQUAL(CommandExecutionState::Running, result1.state);
    TEST_ASSERT_EQUAL(0, result1.exitCode);
    TEST_ASSERT_TRUE(context.variables.find("__cat_path") != context.variables.end());
    TEST_ASSERT_TRUE(context.variables.find("__cat_pos") != context.variables.end());

    // Second invocation
    CommandInvocation invocation2{"cat", "large.txt", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result2 = cat.execute(invocation2);

    TEST_ASSERT_EQUAL(CommandExecutionState::Completed, result2.state);
    TEST_ASSERT_EQUAL(0, result2.exitCode);
}

int process_tests_cat()
{
    UNITY_BEGIN();
    RUN_TEST(test_cat_trigger_small_file);
    RUN_TEST(test_cat_trigger_large_file);
    RUN_TEST(test_cat_trigger_file_not_exists);
    RUN_TEST(test_cat_usage);
    RUN_TEST(test_cat_trigger_edge_cases);
    RUN_TEST(test_cat_execute_small_file_writes_stdout);
    RUN_TEST(test_cat_execute_missing_file_writes_stderr);
    RUN_TEST(test_cat_manual_stream_debug);
    RUN_TEST(test_cat_execute_streaming_simple);
    UNITY_END();
    return 0;
}

#if (defined(ESP_PLATFORM) || defined(ESP32)) && not defined(ARDUINO)
extern "C" void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(4000));
    process_tests_cat();
}
#elif defined(ARDUINO)
void setup()
{
    delay(2500);
    process_tests_cat();
}
void loop() {}
#else
int main()
{
    return process_tests_cat();
}
#endif

// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "../src/commands/cd.h"
#include "../Mocks/MockFileSystem.h"
#include "../Mocks/MockStream.h"
#include "../Mocks/CommandRuntimeTestUtils.h"

void setUp(void) {}
void tearDown(void) {}

void test_cd_trigger_valid_path(void)
{
    MockFileSystem fs;
    fs.createDirectory("/valid");
    DirectoryNavigator dir(&fs);
    cmd::cd cd(dir);
    ETString keyword = "cd";
    ETString additional = "/valid";
    ETString result = cd.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("> valid") != ETString::npos || result.find("\n") != ETString::npos);
}

void test_cd_trigger_invalid_path(void)
{
    MockFileSystem fs;
    DirectoryNavigator dir(&fs);
    cmd::cd cd(dir);
    ETString keyword = "cd";
    ETString additional = "/invalid";
    ETString result = cd.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("did not exist") != ETString::npos);

    // Test with a file path that is not a directory
    fs.createFile("not_a_dir.txt", "content", 7);
    ETString filePath = "not_a_dir.txt";
    ETString result2 = cd.trigger(keyword, filePath);
    TEST_ASSERT_TRUE(result2.find("not a directory") != ETString::npos);
}

void test_cd_usage(void)
{
    MockFileSystem fs;
    DirectoryNavigator dir(&fs);
    cmd::cd cd(dir);
    ETString keyword = "cd";
    ETString result = cd.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Change the current directory") != ETString::npos);
}

void test_cd_empty_keyword(void)
{
    MockFileSystem fs;
    DirectoryNavigator dir(&fs);
    cmd::cd cd(dir);
    ETString keyword = "";
    ETString additional = "";
    ETString result = cd.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("Expected parameter") != ETString::npos);
}

void test_cd_pwd_cd_back_to_pwd(void)
{
    MockFileSystem FS;
    FS.createDirectory("/");
    FS.createDirectory("/folder");
    FS.createDirectory("/folder/another");
    FS.createDirectory("/folder/another/deeper");
    FS.createDirectory("/folder2");
    DirectoryNavigator dir(&FS);
}

void test_cd_auto_completion(void)
{
    MockFileSystem fs;
    fs.createDirectory("/dir1");
    fs.createDirectory("/dir2");
    fs.createDirectory("/dir3");
    DirectoryNavigator dir(&fs);
    cmd::cd cd(dir);

    ETVector<ETString> suggestions = cd.getSuggestions("");
    TEST_ASSERT_EQUAL(3, suggestions.size());
    TEST_ASSERT_TRUE(suggestions[0] == "/dir1/");
    TEST_ASSERT_TRUE(suggestions[1] == "/dir2/");
    TEST_ASSERT_TRUE(suggestions[2] == "/dir3/");
}

void test_cd_auto_completion_partial(void)
{
    MockFileSystem fs;
    fs.createDirectory("/dir1");
    fs.createDirectory("/dir2");
    fs.createDirectory("/dir3");
    DirectoryNavigator dir(&fs);
    cmd::cd cd(dir);

    ETVector<ETString> suggestions = cd.getSuggestions("dir");
    TEST_ASSERT_EQUAL(3, suggestions.size());
    TEST_ASSERT_TRUE(suggestions[0] == "/dir1/");
    TEST_ASSERT_TRUE(suggestions[1] == "/dir2/");
    TEST_ASSERT_TRUE(suggestions[2] == "/dir3/");
}

void test_cd_auto_completion_no_match(void)
{
    MockFileSystem fs;
    fs.createDirectory("/dir1");
    fs.createDirectory("/dir2");
    fs.createDirectory("/dir3");
    DirectoryNavigator dir(&fs);
    cmd::cd cd(dir);

    ETVector<ETString> suggestions = cd.getSuggestions("xyz");
    TEST_ASSERT_EQUAL(0, suggestions.size());
}

void test_cd_stream_output_on_execute(void)
{
    MockFileSystem fs;
    fs.createDirectory("/folder");
    DirectoryNavigator dir(&fs);
    cmd::cd cd(dir);
    MockStream stream;

    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    ETString keyword = "cd";
    ETString arg = "folder";

    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    CommandInvocation invocation{keyword, arg, context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = cd.execute(invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("> /folder") != ETString::npos);
}

void test_cd_stream_output_on_execute_invalid_path(void)
{
    MockFileSystem fs;
    DirectoryNavigator dir(&fs);
    cmd::cd cd(dir);
    MockStream stream;

    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    ETString keyword = "cd";
    ETString arg = "nonexistent";

    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    CommandInvocation invocation{keyword, arg, context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = cd.execute(invocation);

    TEST_ASSERT_EQUAL(1, result.exitCode);
    TEST_ASSERT_TRUE(stream.stderrBuffer.find("did not exist") != ETString::npos);
}

void test_cd_stream_output_on_execute_no_parameter(void)
{
    MockFileSystem fs;
    DirectoryNavigator dir(&fs);
    cmd::cd cd(dir);
    MockStream stream;

    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    ETString keyword = "cd";
    ETString arg = "   ";

    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    CommandInvocation invocation{keyword, arg, context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = cd.execute(invocation);

    TEST_ASSERT_EQUAL(1, result.exitCode);
    TEST_ASSERT_TRUE(stream.stderrBuffer.find("Expected parameter") != ETString::npos);
}

int process_tests_cd()
{

#ifndef COMBINED_TESTS
    UNITY_BEGIN();
#endif
    RUN_TEST(test_cd_trigger_valid_path);
    RUN_TEST(test_cd_trigger_invalid_path);
    RUN_TEST(test_cd_usage);
    RUN_TEST(test_cd_empty_keyword);
    RUN_TEST(test_cd_pwd_cd_back_to_pwd);
    RUN_TEST(test_cd_auto_completion);
    RUN_TEST(test_cd_auto_completion_partial);
    RUN_TEST(test_cd_auto_completion_no_match);
    RUN_TEST(test_cd_stream_output_on_execute);
    RUN_TEST(test_cd_stream_output_on_execute_invalid_path);
    RUN_TEST(test_cd_stream_output_on_execute_no_parameter);

#ifndef COMBINED_TESTS
    UNITY_END();
#endif
    return 0;
}
#if (defined(ESP_PLATFORM) || defined(ESP32)) && not defined(ARDUINO)
extern "C" void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(4000));
    process_tests_cd();
}
#elif defined(ARDUINO)
void setup()
{
    delay(2500);
    process_tests_cd();
}
void loop() {}
#else
int main()
{
    return process_tests_cd();
}
#endif

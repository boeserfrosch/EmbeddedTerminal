// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "../src/commands/pwd.h"
#include "../Mocks/MockFileSystem.h"
#include "../Mocks/MockStream.h"
#include "../Mocks/CommandRuntimeTestUtils.h"

void setUp(void) {}
void tearDown(void) {}

void test_pwd_returns_root_directory(void)
{
    MockFileSystem fs;
    DirectoryNavigator dir(&fs);
    cmd::pwd pwd(dir);
    ETString keyword = "pwd";
    ETString additional = "";
    ETString result = pwd.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("/") != ETString::npos);
}

void test_pwd_returns_changed_directory(void)
{
    MockFileSystem fs;
    fs.createDirectory("/home");
    DirectoryNavigator dir(&fs);
    cmd::pwd pwd(dir);
    
    // Change to /home
    dir.cd("/home");
    ETString keyword = "pwd";
    ETString additional = "";
    ETString result = pwd.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("home") != ETString::npos);
}

void test_pwd_ignores_additional_parameters(void)
{
    MockFileSystem fs;
    fs.createDirectory("/test");
    DirectoryNavigator dir(&fs);
    cmd::pwd pwd(dir);
    
    dir.cd("/test");
    ETString keyword = "pwd";
    ETString additional = "extra params that should be ignored";
    ETString result = pwd.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("test") != ETString::npos);
}

void test_pwd_usage(void)
{
    MockFileSystem fs;
    DirectoryNavigator dir(&fs);
    cmd::pwd pwd(dir);
    ETString keyword = "pwd";
    ETString result = pwd.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Print the current working directory") != ETString::npos);
}

void test_pwd_with_nested_directories(void)
{
    MockFileSystem fs;
    fs.createDirectory("/usr");
    fs.createDirectory("/usr/local");
    fs.createDirectory("/usr/local/bin");
    DirectoryNavigator dir(&fs);
    cmd::pwd pwd(dir);
    
    dir.cd("/usr");
    dir.cd("local");
    dir.cd("bin");
    ETString keyword = "pwd";
    ETString additional = "";
    ETString result = pwd.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("bin") != ETString::npos);
}

void test_pwd_returns_string_ending_with_newline(void)
{
    MockFileSystem fs;
    DirectoryNavigator dir(&fs);
    cmd::pwd pwd(dir);
    ETString keyword = "pwd";
    ETString additional = "";
    ETString result = pwd.trigger(keyword, additional);
    TEST_ASSERT_EQUAL('\n', result[result.length() - 1]);
}

void test_pwd_execute_writes_stdout(void)
{
    MockFileSystem fs;
    DirectoryNavigator dir(&fs);
    cmd::pwd pwd(dir);

    MockStream stream;
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{"pwd", "", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = pwd.execute(invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("/") != ETString::npos);
    TEST_ASSERT_TRUE(stream.stderrBuffer.empty());
}

int process_tests_pwd()
{

#ifndef COMBINED_TESTS
    UNITY_BEGIN();
#endif
    RUN_TEST(test_pwd_returns_root_directory);
    RUN_TEST(test_pwd_returns_changed_directory);
    RUN_TEST(test_pwd_ignores_additional_parameters);
    RUN_TEST(test_pwd_usage);
    RUN_TEST(test_pwd_with_nested_directories);
    RUN_TEST(test_pwd_returns_string_ending_with_newline);
    RUN_TEST(test_pwd_execute_writes_stdout);
#ifndef COMBINED_TESTS
    UNITY_END();
#endif
    return 0;
}
#if (defined(ESP_PLATFORM) || defined(ESP32)) && not defined(ARDUINO)
extern "C" void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(4000));
    process_tests_pwd();
}
#elif defined(ARDUINO)
void setup()
{
    delay(2000);
    process_tests_pwd();
}
void loop() {}
#else // Native
int main()
{
    return process_tests_pwd();
}
#endif

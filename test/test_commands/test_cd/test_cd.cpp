// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "../../../src/commands/cd.h"
#include "../../Mocks/native/MockFileSystem.h"
#include "../../Mocks/MockStream.h"
#include "../../Mocks/CommandRuntimeTestUtils.h"
#include "StorageSystem.h"
#include "../../Mocks/native/MockStorageMedia.h"

IStorageSystem *storage = nullptr;

void setUp(void)
{
    storage = new StorageSystem();
    auto media = new MockStorageMedia("mock", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storage->mountMedia(media, "/");
}
void tearDown(void)
{
    auto medias = storage->media();
    for (auto media : medias)
    {
        storage->unmountMedia(media->name());
        delete media;
    }
    delete storage;
    storage = nullptr;
}

void test_cd_trigger_valid_path(void)
{
    storage->mkdir("/valid");
    DirectoryNavigator dir(storage);
    cmd::cd cd(dir);
    ETString keyword = "cd";
    ETString additional = "/valid";
    ETString result = cd.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("> valid") != ETString::npos || result.find("\n") != ETString::npos);
}

void test_cd_trigger_invalid_path(void)
{
    DirectoryNavigator dir(storage);
    cmd::cd cd(dir);
    ETString keyword = "cd";
    ETString additional = "/invalid";
    ETString result = cd.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("did not exist") != ETString::npos);

    // Test with a file path that is not a directory
    storage->open("/not_a_dir.txt", "w", true).writeAll("content");
    ETString filePath = "/not_a_dir.txt";
    ETString result2 = cd.trigger(keyword, filePath);
    TEST_ASSERT_TRUE(result2.find("not a directory") != ETString::npos);
}

void test_cd_usage(void)
{
    DirectoryNavigator dir(storage);
    cmd::cd cd(dir);
    ETString keyword = "cd";
    ETString result = cd.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Change the current directory") != ETString::npos);
}

void test_cd_empty_keyword(void)
{
    DirectoryNavigator dir(storage);
    cmd::cd cd(dir);
    ETString keyword = "";
    ETString additional = "";
    ETString result = cd.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("Expected parameter") != ETString::npos);
}

void test_cd_pwd_cd_back_to_pwd(void)
{
    storage->mkdir("/");
    storage->mkdir("/folder");
    storage->mkdir("/folder/another");
    storage->mkdir("/folder/another/deeper");
    storage->mkdir("/folder2");
    DirectoryNavigator dir(storage);
    cmd::cd cd(dir);
    // Start at root
    TEST_ASSERT_TRUE(dir.pwd() == "/");
    TEST_ASSERT_TRUE(dir.pwd().isRoot());
    // cd into folder
    cd.trigger("cd", "folder");
    TEST_ASSERT_TRUE(dir.pwd() == "/folder");
    // cd into another
    cd.trigger("cd", "another");
    TEST_ASSERT_TRUE(dir.pwd() == "/folder/another");
    // cd into deeper
    cd.trigger("cd", "deeper");
    TEST_ASSERT_TRUE(dir.pwd() == "/folder/another/deeper");
    // cd back to another
    cd.trigger("cd", "..");
    TEST_ASSERT_TRUE(dir.pwd() == "/folder/another");
    // cd back to folder
    cd.trigger("cd", "..");
    TEST_ASSERT_TRUE(dir.pwd() == "/folder");
    // cd staying in folder
    cd.trigger("cd", ".");
    TEST_ASSERT_TRUE(dir.pwd() == "/folder");
    // cd back to root
    cd.trigger("cd", "/");
    TEST_ASSERT_TRUE(dir.pwd() == "/");
    TEST_ASSERT_TRUE(dir.pwd().isRoot());
}

void test_cd_auto_completion(void)
{
    storage->mkdir("/dir1");
    storage->mkdir("/dir2");
    storage->mkdir("/dir3");
    DirectoryNavigator dir(storage);
    cmd::cd cd(dir);

    ETVector<ETString> suggestions = cd.getSuggestions("");
    TEST_ASSERT_EQUAL(3, suggestions.size());
    TEST_ASSERT_TRUE(suggestions[0] == "/dir1/");
    TEST_ASSERT_TRUE(suggestions[1] == "/dir2/");
    TEST_ASSERT_TRUE(suggestions[2] == "/dir3/");
}

void test_cd_auto_completion_partial(void)
{
    storage->mkdir("/dir1");
    storage->mkdir("/dir2");
    storage->mkdir("/dir3");
    DirectoryNavigator dir(storage);
    cmd::cd cd(dir);

    ETVector<ETString> suggestions = cd.getSuggestions("dir");
    TEST_ASSERT_EQUAL(3, suggestions.size());
    TEST_ASSERT_TRUE(suggestions[0] == "/dir1/");
    TEST_ASSERT_TRUE(suggestions[1] == "/dir2/");
    TEST_ASSERT_TRUE(suggestions[2] == "/dir3/");
}

void test_cd_auto_completion_no_match(void)
{
    storage->mkdir("/dir1");
    storage->mkdir("/dir2");
    storage->mkdir("/dir3");
    DirectoryNavigator dir(storage);
    cmd::cd cd(dir);

    ETVector<ETString> suggestions = cd.getSuggestions("xyz");
    TEST_ASSERT_EQUAL(0, suggestions.size());
}

void test_cd_stream_output_on_execute(void)
{
    storage->mkdir("/folder");
    DirectoryNavigator dir(storage);
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
    storage->mkdir("/valid");
    DirectoryNavigator dir(storage);
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
    storage->mkdir("/test");
    DirectoryNavigator dir(storage);
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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
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

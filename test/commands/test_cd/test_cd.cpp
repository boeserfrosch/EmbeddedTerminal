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
#include "../../Mocks/CommandRuntimeTestUtils.h"
#include "StorageSystem.h"
#include "../../Mocks/native/MockStorageMedia.h"
#include "../utils.h"

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
    auto invocationHandle = TestCommandInvocationHandle(keyword, {additional});
    CommandResult result = cd.invoke(invocationHandle.invocation);
    TEST_ASSERT_TRUE(invocationHandle.output.contains("> /valid"));
}

void test_cd_trigger_invalid_path(void)
{
    DirectoryNavigator dir(storage);
    cmd::cd cd(dir);
    ETString keyword = "cd";
    ETString additional = "/invalid";
    auto invocationHandle = TestCommandInvocationHandle(keyword, {additional});
    CommandResult result = cd.invoke(invocationHandle.invocation);
    TEST_ASSERT_EQUAL(cmd::cd::ErrorCode::PathDoesNotExist, result.exitCode);
    TEST_ASSERT_TRUE(invocationHandle.error.contains("Path does not exist"));

    // Test with a file path that is not a directory
    storage->open("/not_a_dir.txt", "w", true).writeAll("content");
    ETString filePath = "/not_a_dir.txt";
    auto invocationHandle2 = TestCommandInvocationHandle(keyword, {filePath});
    CommandResult result2 = cd.invoke(invocationHandle2.invocation);
    TEST_ASSERT_EQUAL(cmd::cd::ErrorCode::PathNotDirectory, result2.exitCode);
    TEST_ASSERT_TRUE(invocationHandle2.error.contains("Path is not a directory"));
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
    auto invocationHandle = TestCommandInvocationHandle(keyword, {});
    CommandResult result = cd.invoke(invocationHandle.invocation);
    TEST_ASSERT_TRUE(invocationHandle.output.contains(cd.usage(keyword)));
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
    auto iHandle = TestCommandInvocationHandle("cd", {"folder"});
    CommandResult result = cd.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(dir.pwd() == "/folder");
    // cd into another
    auto iHandle2 = TestCommandInvocationHandle("cd", {"another"});
    CommandResult result2 = cd.invoke(iHandle2.invocation);
    TEST_ASSERT_TRUE(dir.pwd() == "/folder/another");
    // cd into deeper
    auto iHandle3 = TestCommandInvocationHandle("cd", {"deeper"});
    CommandResult result3 = cd.invoke(iHandle3.invocation);
    TEST_ASSERT_TRUE(dir.pwd() == "/folder/another/deeper");
    // cd back to another
    auto iHandle4 = TestCommandInvocationHandle("cd", {".."});
    CommandResult result4 = cd.invoke(iHandle4.invocation);
    TEST_ASSERT_TRUE(dir.pwd() == "/folder/another");
    // cd back to folder
    auto iHandle5 = TestCommandInvocationHandle("cd", {".."});
    CommandResult result5 = cd.invoke(iHandle5.invocation);
    TEST_ASSERT_TRUE(dir.pwd() == "/folder");
    // cd staying in folder
    auto iHandle6 = TestCommandInvocationHandle("cd", {});
    CommandResult result6 = cd.invoke(iHandle6.invocation);
    TEST_ASSERT_TRUE(dir.pwd() == "/folder");
    // cd back to root
    auto iHandle7 = TestCommandInvocationHandle("cd", {"/"});
    CommandResult result7 = cd.invoke(iHandle7.invocation);
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

    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    ETString keyword = "cd";
    ETVector<ETString> arg = {"folder"};

    auto invocationHandle = TestCommandInvocationHandle(keyword, arg);
    CommandResult result = cd.invoke(invocationHandle.invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(invocationHandle.output.contains("> /folder"));
}

void test_cd_stream_output_on_execute_invalid_path(void)
{
    storage->mkdir("/valid");
    DirectoryNavigator dir(storage);
    cmd::cd cd(dir);

    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    ETString keyword = "cd";
    ETVector<ETString> arg = {"nonexistent"};

    auto iHandle = TestCommandInvocationHandle(keyword, arg);
    CommandResult result = cd.invoke(iHandle.invocation);

    TEST_ASSERT_EQUAL(1, result.exitCode);
    TEST_MESSAGE(("Result: " + iHandle.error.debugOutput).c_str());
    TEST_ASSERT_TRUE(iHandle.error.contains("does not exist"));
}

void test_cd_stream_output_on_execute_no_parameter(void)
{
    storage->mkdir("/test");
    DirectoryNavigator dir(storage);
    cmd::cd cd(dir);

    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    ETString keyword = "cd";
    ETVector<ETString> arg = {}; // No argument provided

    auto iHandle = TestCommandInvocationHandle(keyword, arg);
    CommandResult result = cd.invoke(iHandle.invocation);

    TEST_ASSERT_EQUAL(1, result.exitCode);
    TEST_ASSERT_TRUE(iHandle.output.contains(cd.usage(keyword)));
}

void test_cd_correct_error_codes_and_messages(void)
{
    storage->mkdir("/test");
    storage->open("/test/file.txt", "w", true).writeAll("content");
    DirectoryNavigator dir(storage);
    cmd::cd cd(dir);

    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    ETString keyword = "cd";
    ETVector<ETString> arg = {}; // No argument provided

    auto iHandle = TestCommandInvocationHandle(keyword, arg);
    CommandResult result = cd.invoke(iHandle.invocation);

    TEST_ASSERT_EQUAL(1, result.exitCode);
    TEST_ASSERT_TRUE(iHandle.output.contains(cd.usage(keyword)));

    // Test with non-existent path
    arg = {"/nonexistent"};
    auto iHandle2 = TestCommandInvocationHandle(keyword, arg);
    CommandResult result2 = cd.invoke(iHandle2.invocation);

    TEST_ASSERT_EQUAL(cmd::cd::ErrorCode::PathDoesNotExist, result2.exitCode);
    TEST_ASSERT_TRUE(iHandle2.error.contains("does not exist"));

    // Test with a file path that is not a directory
    arg = {"/test/file.txt"};
    auto iHandle3 = TestCommandInvocationHandle(keyword, arg);
    CommandResult result3 = cd.invoke(iHandle3.invocation);
    TEST_ASSERT_EQUAL(cmd::cd::ErrorCode::PathNotDirectory, result3.exitCode);
    TEST_ASSERT_TRUE(iHandle3.error.contains("is not a directory"));

    // Test with valid path
    arg = {"/test"};
    auto iHandle4 = TestCommandInvocationHandle(keyword, arg);
    CommandResult result4 = cd.invoke(iHandle4.invocation);
    TEST_ASSERT_EQUAL(cmd::cd::ErrorCode::None, result4.exitCode);
    TEST_ASSERT_TRUE(iHandle4.output.contains("> /test"));
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
    RUN_TEST(test_cd_correct_error_codes_and_messages);

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

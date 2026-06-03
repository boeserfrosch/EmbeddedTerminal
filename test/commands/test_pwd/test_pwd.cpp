// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "../../../src/commands/pwd.h"
#include "../../Mocks/native/MockFileSystem.h"
#include "../utils.h"
#include "StorageSystem.h"
#include "../../Mocks/native/MockStorageMedia.h"

IStorageSystem *storage = nullptr;
DirectoryNavigator *dir = nullptr;

void setUp(void)
{
    storage = new StorageSystem();
    auto media = new MockStorageMedia("mock", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storage->mountMedia(media, "/");
    dir = new DirectoryNavigator(storage);
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

void test_pwd_returns_root_directory(void)
{
    cmd::pwd pwd(*dir);
    ETString keyword = "pwd";
    TestCommandInvocationHandle iHandle("pwd");
    ;
    CommandResult result = pwd.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains("/"));
}

void test_pwd_returns_changed_directory(void)
{
    storage->mkdir("/home");
    cmd::pwd pwd(*dir);

    // Change to /home
    dir->cd("/home");
    ETString keyword = "pwd";
    ETString additional = "";
    TestCommandInvocationHandle iHandle("pwd");
    ;
    CommandResult result = pwd.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains("home"));
}

void test_pwd_ignores_additional_parameters(void)
{
    storage->mkdir("/test");
    cmd::pwd pwd(*dir);

    dir->cd("/test");
    ETString keyword = "pwd";
    ETString additional = "extra params that should be ignored";
    TestCommandInvocationHandle iHandle("pwd");
    ;
    CommandResult result = pwd.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains("test"));
}

void test_pwd_usage(void)
{
    cmd::pwd pwd(*dir);
    ETString keyword = "pwd";
    ETString result = pwd.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Print the current working directory") != ETString::npos);
}

void test_pwd_with_nested_directories(void)
{
    storage->mkdir("/usr");
    storage->mkdir("/usr/local");
    storage->mkdir("/usr/local/bin");
    cmd::pwd pwd(*dir);

    dir->cd("/usr");
    dir->cd("local");
    dir->cd("bin");
    ETString keyword = "pwd";
    ETString additional = "";
    TestCommandInvocationHandle iHandle("pwd");
    ;
    CommandResult result = pwd.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains("bin"));
}

void test_pwd_returns_string_ending_with_newline(void)
{
    cmd::pwd pwd(*dir);
    ETString keyword = "pwd";
    TestCommandInvocationHandle iHandle("pwd");
    ;
    CommandResult result = pwd.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.endsWith("\n"));
}

void test_pwd_execute_writes_stdout(void)
{
    cmd::pwd pwd(*dir);

    TestCommandInvocationHandle iHandle("pwd");
    ;
    CommandResult result = pwd.invoke(iHandle.invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(iHandle.output.contains("/"));
    TEST_ASSERT_TRUE(iHandle.error.empty());
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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
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

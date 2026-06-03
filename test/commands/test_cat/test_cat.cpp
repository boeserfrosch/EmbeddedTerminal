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
#include "../../../src/DirectoryNavigator.h"
#include "../../Mocks/native/MockFileSystem.h"
#include "../../Mocks/MockStream.h"
#include "../../Mocks/CommandRuntimeTestUtils.h"
#include "StorageSystem.h"
#include "../../Mocks/native/MockStorageMedia.h"
#include "../utils.h"
#include <memory>

#include <unity.h>

IStorageSystem *storage = nullptr;
static std::shared_ptr<MockStorageMedia> media;

void setUp(void)
{
    storage = new StorageSystem();
    media = std::make_shared<MockStorageMedia>("mock", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storage->mountMedia(media, "/");
}
void tearDown(void)
{
    if (media)
    {
        storage->unmountMedia(media->name());
        media.reset();
    }
    delete storage;
    storage = nullptr;
}

void test_cat_small_file(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage);
    storage->open("/file.txt", "w", true).writeAll("hello1234"); // Small file
    EmbeddedTerminal::cmd::cat cat(dir);

    ETString keyword = "cat";
    ETString arg = "file.txt";
    TestCommandInvocationHandle invocationHandle(keyword, {arg});
    ;
    CommandResult result = cat.invoke(invocationHandle.invocation);
    TEST_ASSERT_EQUAL(CommandExecutionState::Completed, result.state);
    TEST_ASSERT_TRUE(invocationHandle.output.contains("hello1234"));
}

void test_cat_large_file(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage);
    auto file = storage->open("/big.txt", "w", true);
    for (int i = 0; i < 1000; i++)
    {
        file.writeAll("0123456789"); // 10KB total
    }
    EmbeddedTerminal::cmd::cat cat(dir);
    ETString keyword = "cat";
    ETString arg = "big.txt";
    TestCommandInvocationHandle invocationHandle(keyword, {arg});
    CommandResult result = cat.invoke(invocationHandle.invocation);
    TEST_ASSERT_TRUE(dir.exists("/big.txt"));
    TEST_ASSERT_TRUE(dir.exists("big.txt"));
    TEST_ASSERT_EQUAL(CommandExecutionState::Running, result.state);
}

void test_cat_file_not_exists(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage);
    EmbeddedTerminal::cmd::cat cat(dir);

    ETString keyword = "cat";
    ETString arg = "nofile.txt";
    TestCommandInvocationHandle invocationHandle(keyword, {arg});
    CommandResult result = cat.invoke(invocationHandle.invocation);
    TEST_ASSERT_TRUE(invocationHandle.error.contains("did not exist"));
}

void test_cat_usage(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage);
    EmbeddedTerminal::cmd::cat cat(dir);
    ETString keyword = "cat";
    ETString result = cat.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Returns the content") != ETString::npos);
}

void test_cat_edge_cases(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage);
    EmbeddedTerminal::cmd::cat cat(dir);
    ETString keyword = "cat";
    TestCommandInvocationHandle invocationHandle(keyword);
    CommandResult result = cat.invoke(invocationHandle.invocation);
    TEST_ASSERT_TRUE(invocationHandle.output.contains(cat.usage(keyword)));
    ETString arg2 = "did_not_exist.txt";
    TestCommandInvocationHandle invocationHandle2(keyword, {arg2});
    CommandResult result2 = cat.invoke(invocationHandle2.invocation);
    TEST_ASSERT_TRUE(invocationHandle2.error.contains("did not exist"));
}

void test_cat_small_file_writes_stdout(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage);
    storage->open("/file.txt", "w", true).writeAll("hello1234");
    EmbeddedTerminal::cmd::cat cat(dir);

    ETString keyword = "cat";
    ETVector<ETString> arg = {"file.txt"};

    TestCommandInvocationHandle invocationHandle(keyword, arg);
    CommandResult result = cat.invoke(invocationHandle.invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(invocationHandle.output.contains("hello1234"));
    TEST_ASSERT_TRUE(invocationHandle.error.empty());
}

void test_cat_missing_file_writes_stderr(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage);
    EmbeddedTerminal::cmd::cat cat(dir);

    ETString keyword = "cat";
    ETVector<ETString> arg = {"missing.txt"};

    TestCommandInvocationHandle invocationHandle(keyword, arg);
    CommandResult result = cat.invoke(invocationHandle.invocation);

    TEST_ASSERT_EQUAL(2, result.exitCode);
    TEST_ASSERT_TRUE(invocationHandle.error.contains("did not exist"));
    TEST_ASSERT_TRUE(invocationHandle.output.empty());
}

void test_cat_manual_stream_debug(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage);
    ETString largeContent = "";
    for (int i = 0; i < 30; i++)
    {
        largeContent += "0123456789";
    }
    largeContent += "Extra";
    auto file = storage->open("/large.txt", "w", true);
    file.writeAll(largeContent);
    file.close();

    // Manually read the file Chunk by chunk
    auto file1 = dir.open("large.txt", "r", false);
    TEST_ASSERT_TRUE(file1.isOpen());
    TEST_ASSERT_EQUAL(0, file1.position());

    unsigned char buf1[256];
    size_t read1 = file1.read(buf1, 256);
    TEST_ASSERT_EQUAL_INT(256, read1);
    file1.close();

    // Second read should start from beginning
    auto file2 = dir.open("large.txt", "r", false);
    TEST_ASSERT_TRUE(file2.isOpen());
    file2.seek(256); // Seek to where first read ended

    unsigned char buf2[256];
    size_t read2 = file2.read(buf2, 256);
    TEST_ASSERT_EQUAL_INT(49, read2); // Should read remaining 49 bytes
    file2.close();
}

void test_cat_execute_streaming_simple(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage);
    ETString largeContent = "";
    for (int i = 0; i < 90; i++)
    {
        largeContent += "0123456789";
    }
    largeContent += "Extra";
    auto file = storage->open("/large.txt", "w", true);
    file.writeAll(largeContent);
    file.close();

    EmbeddedTerminal::cmd::cat cat(dir);
    // First invocation
    TestCommandInvocationHandle invocationHandle("cat", {"large.txt"});
    CommandResult result1 = cat.invoke(invocationHandle.invocation);

    TEST_ASSERT_EQUAL(CommandExecutionState::Running, result1.state);
    TEST_ASSERT_EQUAL(0, result1.exitCode);
    TEST_ASSERT_TRUE(invocationHandle.context.variables.find("cat__path") != invocationHandle.context.variables.end());
    TEST_ASSERT_TRUE(invocationHandle.context.variables.find("cat__pos") != invocationHandle.context.variables.end());
    TEST_ASSERT_TRUE(invocationHandle.output.debugOutput != largeContent);

    // Second invocation
    CommandResult result2 = cat.resume(invocationHandle.invocation);

    TEST_ASSERT_EQUAL(CommandExecutionState::Completed, result2.state);
    TEST_ASSERT_EQUAL(0, result2.exitCode);
    TEST_ASSERT_TRUE(invocationHandle.output.contains(largeContent));
}

int process_tests_cat()
{
    UNITY_BEGIN();
    RUN_TEST(test_cat_small_file);
    RUN_TEST(test_cat_large_file);
    RUN_TEST(test_cat_file_not_exists);
    RUN_TEST(test_cat_usage);
    RUN_TEST(test_cat_edge_cases);
    RUN_TEST(test_cat_small_file_writes_stdout);
    RUN_TEST(test_cat_missing_file_writes_stderr);
    RUN_TEST(test_cat_manual_stream_debug);
    RUN_TEST(test_cat_execute_streaming_simple);
    UNITY_END();
    return 0;
}

#if (defined(ESP_PLATFORM) || defined(ESP32)) && not defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
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

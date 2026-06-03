#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "../../Mocks/native/MockFileSystem.h"
#include "../../Mocks/CommandRuntimeTestUtils.h"
#include "commands/download.h"
#include "StorageSystem.h"
#include "../../Mocks/native/MockStorageMedia.h"
#include "../utils.h"
#include <memory>
#include <vector>

static std::unique_ptr<IStorageSystem> storage;

void setUp(void)
{
    storage.reset(new StorageSystem());
    TEST_ASSERT_NOT_NULL(storage.get());
    // create media and mount it. test harness will request unmount via StorageSystem
    auto media = std::make_shared<MockStorageMedia>("mock", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storage->mountMedia(media, "/");
}
void tearDown(void)
{
    if (!storage)
        return;

    // Ensure we unmount any mounted media. Do NOT delete the media pointers here
    // to avoid potential double-free if StorageSystem owns them.
    auto medias = storage->media();
    for (auto media : medias)
    {
        storage->unmountMedia(media->name());
    }

    storage.reset();
}

void test_download_basic(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage.get());
    storage->open("/file.txt", "w", true).writeAll("hello1234"); // Small file
    EmbeddedTerminal::cmd::download download(dir);

    TestCommandInvocationHandle iHandle("download", {"file.txt"});
    ;
    CommandResult result = download.invoke(iHandle.invocation);
    TEST_ASSERT_EQUAL(CommandExecutionState::Completed, result.state);

    TEST_ASSERT_TRUE(iHandle.output.contains("SIZE 9"));
    TEST_ASSERT_TRUE(iHandle.output.contains("aGVsbG8xMjM0"));
    TEST_ASSERT_TRUE(iHandle.output.contains("EOF"));
}

void test_download_invalid_path(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage.get());
    EmbeddedTerminal::cmd::download download(dir);
    TestCommandInvocationHandle iHandle("download", {});
    ;
    CommandResult result = download.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains(download.usage("download")));
}

void test_download_file_not_found(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage.get());
    EmbeddedTerminal::cmd::download download(dir);
    TestCommandInvocationHandle iHandle("download", {"nofile.txt"});
    ;
    CommandResult result = download.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.error.contains("did not exist"));
    TEST_ASSERT_TRUE(iHandle.output.empty());
}

void test_download_is_directory(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage.get());
    storage->mkdir("/mydir");
    EmbeddedTerminal::cmd::download download(dir);
    TestCommandInvocationHandle iHandle("download", {"mydir"});
    ;
    CommandResult result = download.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.error.contains("is a directory"));
    TEST_ASSERT_TRUE(iHandle.output.empty());
}

void test_download_streaming(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage.get());
    ETString bigfileContent = "";
    for (int i = 0; i < 600; i++)
    {
        bigfileContent += "ABCDEFGHIJ";
    }
    storage->open("/bigfile.txt", "w", true).writeAll(bigfileContent);

    EmbeddedTerminal::cmd::download download(dir);

    TestCommandInvocationHandle iHandle("download", {"bigfile.txt"});
    CommandResult result = download.invoke(iHandle.invocation);

    TEST_ASSERT_MESSAGE(iHandle.output.contains("SIZE 6000"), "Expected SIZE header with file size");
    TEST_ASSERT_MESSAGE(iHandle.output.contains("CHUNK 1/16") || result.state == CommandExecutionState::Completed, "Expected first chunk header or immediate completion");

    // Simulate subsequent calls to complete the streaming. Track chunk index consistently.
    size_t chunkIndex = 1; // already received first chunk in initial output
    size_t safety = 0;
    while (result.state == CommandExecutionState::Running && chunkIndex <= 20)
    {
        result = download.resume(iHandle.invocation);
        TEST_ASSERT_EQUAL(0, result.exitCode);
        safety++;
        if (safety > 32)
        {
            TEST_FAIL_MESSAGE("Too many chunks, possible infinite loop");
            break;
        }

        // If the command is still running, expect an additional chunk header for the next chunk
        chunkIndex++;
        ETString expectedHeader = "CHUNK " + toETString(chunkIndex) + "/16";
        TEST_ASSERT_TRUE_MESSAGE(iHandle.output.contains(expectedHeader), "Expected chunk header in streaming response");

        // During running state, EOF must not yet be present
        if (result.state == CommandExecutionState::Running)
        {
            TEST_ASSERT_FALSE_MESSAGE(iHandle.output.contains("EOF"), "Should not find EOF before the last chunk");
        }
    }

    // After loop completes, ensure final state is Completed and EOF present
    TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Completed, "Expected final state to be Completed after last chunk");
    TEST_ASSERT_MESSAGE(iHandle.output.contains("EOF"), "Expected EOF in the final response");
}

void test_download_streaming_error(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage.get());
    EmbeddedTerminal::cmd::download download(dir);
    TestCommandInvocationHandle iHandle("download", {"nofile.txt"});
    ;
    CommandResult result = download.invoke(iHandle.invocation);
    TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Completed, "Expected state to be Completed on error");
    TEST_ASSERT_MESSAGE(result.exitCode != 0, "Expected non-zero exit code on error");
    TEST_ASSERT_MESSAGE(result.exitCode == EmbeddedTerminal::cmd::download::ErrorCode::FILE_NOT_FOUND, "Expected specific error code for failed file open");
    TEST_ASSERT_MESSAGE(iHandle.error.contains("did not exist"), "Expected error message about file not existing");
    TEST_ASSERT_FALSE_MESSAGE(iHandle.output.contains("EOF"), "Should not find EOF in error case");
    TEST_ASSERT_TRUE(true);
}

void test_download_usage(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage.get());
    EmbeddedTerminal::cmd::download download(dir);
    ETString usage = download.usage("download");
    TEST_ASSERT_TRUE(usage.find("Download a specific file") != ETString::npos);
    TEST_ASSERT_TRUE(usage.find("download <path>") != ETString::npos);
}

void test_download_auto_completion(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage.get());
    storage->open("/file1.txt", "w", true).writeAll("content");
    storage->open("/file2.txt", "w", true).writeAll("content");
    storage->mkdir("/mydir");
    EmbeddedTerminal::cmd::download download(dir);

    ETVector<ETString> suggestions = download.getSuggestions("f");
    TEST_ASSERT_TRUE(suggestions.size() >= 2);
    TEST_ASSERT_TRUE(suggestions[0].contains("file1.txt") || suggestions[0].contains("file2.txt"));
    TEST_ASSERT_TRUE(suggestions[1].contains("file1.txt") || suggestions[1].contains("file2.txt"));

    // Should also suggest directories, cause there could be files in there
    suggestions = download.getSuggestions("my");
    TEST_ASSERT_TRUE(suggestions.size() == 1);
    TEST_ASSERT_TRUE(suggestions[0].contains("mydir"));
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_download_basic);
    RUN_TEST(test_download_invalid_path);
    RUN_TEST(test_download_file_not_found);
    RUN_TEST(test_download_is_directory);
    RUN_TEST(test_download_streaming);
    RUN_TEST(test_download_streaming_error);
    RUN_TEST(test_download_usage);
    RUN_TEST(test_download_auto_completion);
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

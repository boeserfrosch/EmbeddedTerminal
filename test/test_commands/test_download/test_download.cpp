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

void test_download_basic(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage);
    storage->open("/file.txt", "w", true).writeAll("hello1234"); // Small file
    EmbeddedTerminal::cmd::download download(dir);

    TEST_ASSERT_TRUE(true);
    auto result = download.trigger("download", "file.txt");
    TEST_ASSERT_TRUE(result.find("SIZE 9") != ETString::npos);
    TEST_ASSERT_TRUE(result.find("aGVsbG8xMjM0") != ETString::npos);
    TEST_ASSERT_TRUE(result.find("EOF") != ETString::npos);
}

void test_download_invalid_path(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage);
    EmbeddedTerminal::cmd::download download(dir);
    auto result = download.trigger("download", "   ");
    TEST_ASSERT_TRUE(result.find("Expected parameter") != ETString::npos);
}

void test_download_file_not_found(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage);
    EmbeddedTerminal::cmd::download download(dir);
    auto result = download.trigger("download", "nofile.txt");
    TEST_ASSERT_TRUE(result.find("did not exist") != ETString::npos);
}

void test_download_is_directory(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage);
    storage->mkdir("/mydir");
    EmbeddedTerminal::cmd::download download(dir);
    auto result = download.trigger("download", "mydir");
    TEST_ASSERT_TRUE(result.find("is a directory") != ETString::npos);
}

void test_download_streaming(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage);
    ETString bigfileContent = "";
    for (int i = 0; i < 600; i++)
    {
        bigfileContent += "ABCDEFGHIJ";
    }
    storage->open("/bigfile.txt", "w", true).writeAll(bigfileContent);

    EmbeddedTerminal::cmd::download download(dir);

    MockStream stream;
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};

    CommandInvocation invocation{"download", "bigfile.txt", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = download.execute(invocation);

    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("SIZE 6000") != ETString::npos, "Expected SIZE header with file size");
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("CHUNK 0/15") != ETString::npos, "Expected first chunk header");
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("QUJDREVGR0hJSkFCQ0RFRkdISUpBQkNERUZHS") != ETString::npos, "Expected base64 content in the response");
    TEST_ASSERT_EQUAL(CommandExecutionState::Running, result.state);

    // Simulate subsequent calls to complete the streaming
    size_t chunkCount = 0;
    while (result.state == CommandExecutionState::Running)
    {

        chunkCount++;
        result = download.execute(invocation);
        if (chunkCount < 16)
        {
            TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Running || result.state == CommandExecutionState::Completed, "Expected state to be Running or Completed");
            // Test chunk headers and content in each response
            size_t chunkIndex = stream.stdoutBuffer.find("CHUNK " + toETString(chunkCount) + "/15");
            TEST_ASSERT_MESSAGE(chunkIndex != ETString::npos, "Expected chunk header in streaming response");
            size_t chunkEnd = stream.stdoutBuffer.find("\n", chunkIndex);
            TEST_ASSERT_MESSAGE(chunkEnd != ETString::npos, "Expected newline after chunk header");
            if (result.state == CommandExecutionState::Running)
            {
                // No EOF yet
                TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("EOF", chunkEnd) == ETString::npos, "Should not find EOF before the last chunk");
            }
        }
        if (chunkCount == 16)
        {
            TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Completed, "Expected final state to be Completed after last chunk");
            TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("EOF") != ETString::npos, "Expected EOF in the final response");
        }
    }
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("EOF") != ETString::npos, "Should be complete yet"); // Should not be complete yet
}

void test_download_streaming_error(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage);
    EmbeddedTerminal::cmd::download download(dir);
    MockStream stream;
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    CommandInvocation invocation{"download", "nonexistent.txt", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = download.execute(invocation);
    TEST_ASSERT_MESSAGE(result.state == CommandExecutionState::Completed, "Expected state to be Completed on error");
    TEST_ASSERT_MESSAGE(result.exitCode != 0, "Expected non-zero exit code on error");
    TEST_ASSERT_MESSAGE(result.exitCode == EmbeddedTerminal::cmd::errorCodes::DOWNLOAD_CMD_ERROR_FILE_NOT_FOUND, "Expected specific error code for failed file open");
    TEST_ASSERT_MESSAGE(stream.stderrBuffer.find("did not exist") != ETString::npos, "Expected error message about file not existing");
    TEST_ASSERT_MESSAGE(stream.stdoutBuffer.find("EOF") == ETString::npos, "Should not find EOF in error case");
    TEST_ASSERT_TRUE(true);
}

void test_download_usage(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage);
    EmbeddedTerminal::cmd::download download(dir);
    ETString usage = download.usage("download");
    TEST_ASSERT_TRUE(usage.find("Download a specific file") != ETString::npos);
    TEST_ASSERT_TRUE(usage.find("download [path]") != ETString::npos);
}

void test_download_auto_completion(void)
{
    EmbeddedTerminal::DirectoryNavigator dir(storage);
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

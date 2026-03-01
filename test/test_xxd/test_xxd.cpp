// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "commands/xxd.h"
#include "../src/DirectoryNavigator.h"
#include "../Mocks/MockStream.h"
#include "../Mocks/CommandRuntimeTestUtils.h"
#include "StorageSystem.h"
#include "../Mocks/MockStorageMedia.h"

#include <unity.h>

IStorageSystem *storage = nullptr;
DirectoryNavigator *dir = nullptr;

void setUp(void)
{
    storage = new StorageSystem();
    auto media = new MockStorageMedia("mock", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storage->mountMedia(media, "");
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

void test_xxd_trigger_small_file(void)
{
    storage->open("/file.txt", "w", true).writeAll("hello1234"); // Small file
    EmbeddedTerminal::cmd::xxd xxd(*dir);

    ETString keyword = "xxd";
    ETString arg = "file.txt";
    ETString result = xxd.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("68 65 6C 6C 6F 31 32 33 34") != ETString::npos); // Hex for "hello123"
}

void test_xxd_trigger_large_file(void)
{
    // Fake large file by writing a small string but pretending it's large. The xxd command should read 512 bytes and not truncate since we simulate a large file.
    auto file = storage->open("/big.txt", "w", true);
    for (int i = 0; i < 512; i++)
    {
        file.write("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 26);
    }
    file.close();

    TEST_ASSERT_TRUE(storage->exists("/big.txt"));
    TEST_ASSERT_TRUE(storage->exists("big.txt"));

    EmbeddedTerminal::cmd::xxd xxd(*dir);
    ETString keyword = "xxd";
    ETString arg = "big.txt";
    ETString result = xxd.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("41 42 43") != ETString::npos);             // Hex for "ABC"
    TEST_ASSERT_TRUE(result.length() > 1000);                                // Should be large since we read 512 bytes and not truncate
    TEST_ASSERT_TRUE(result.find("00000200:") == ETString::npos);            // Should have at most 512 bytes, so offset 200 should not be present
    TEST_ASSERT_TRUE(result.find("... output truncated") != ETString::npos); // Should not be truncated since we read 512 bytes
}

void test_xxd_trigger_file_not_exists(void)
{
    EmbeddedTerminal::cmd::xxd xxd(*dir);

    ETString keyword = "xxd";
    ETString arg = "nofile.txt";
    ETString result = xxd.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("did not exist!") != ETString::npos);
}

void test_xxd_usage(void)
{
    EmbeddedTerminal::cmd::xxd xxd(*dir);

    ETString keyword = "xxd";
    ETString result = xxd.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Returns the content of the defined file as hex dump") != ETString::npos);
}

void test_xxd_trigger_edge_cases(void)
{
    EmbeddedTerminal::cmd::xxd xxd(*dir);

    // Test empty path
    ETString result = xxd.trigger("xxd", "   ");
    TEST_ASSERT_TRUE(result.find("path or name to file expected") != ETString::npos);

    // Test directory instead of file
    storage->mkdir("/dir");
    result = xxd.trigger("xxd", "dir");
    TEST_ASSERT_TRUE(result.find("did not exist!") != ETString::npos);
}

void test_xxd_get_suggestions(void)
{
    storage->open("/file1.txt", "w", true).writeAll("data");
    storage->open("/file2.txt", "w", true).writeAll("data");
    storage->mkdir("/dir");
    EmbeddedTerminal::cmd::xxd xxd(*dir);

    ETVector<ETString> suggestions = xxd.getSuggestions("fi");
    TEST_ASSERT_EQUAL(2, suggestions.size());
    TEST_ASSERT_TRUE(suggestions[0] == "/file1.txt" || suggestions[0] == "/file2.txt");
    TEST_ASSERT_TRUE(suggestions[1] == "/file1.txt" || suggestions[1] == "/file2.txt");
}

void test_xxd_execute_writes_stdout(void)
{
    storage->open("/file.txt", "w", true).writeAll("hello1234");
    EmbeddedTerminal::cmd::xxd xxd(*dir);

    MockStream stream;
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{"xxd", "file.txt", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = xxd.execute(invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("00000000") != ETString::npos);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("68 65 6C 6C 6F") != ETString::npos);
    TEST_ASSERT_TRUE(stream.stderrBuffer.empty());
}

void test_xxd_execute_missing_file_writes_stderr(void)
{
    EmbeddedTerminal::cmd::xxd xxd(*dir);

    MockStream stream;
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{"xxd", "missing.bin", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = xxd.execute(invocation);

    TEST_ASSERT_EQUAL(EmbeddedTerminal::cmd::errorCodes::XXD_CMD_ERROR_FILE_NOT_FOUND, result.exitCode);
    TEST_ASSERT_TRUE(stream.stderrBuffer.find("file not found") != ETString::npos);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.empty());
}

void test_xxd_execute_navigates_on_next_key(void)
{
    ETString content = "";
    for (int i = 0; i < 30; i++)
    {
        content += "0123456789";
    }
    storage->open("/big.bin", "w", true).writeAll(content.c_str());

    EmbeddedTerminal::cmd::xxd xxd(*dir);
    MockStream stream;
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    BufferedInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{"xxd", "big.bin", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult first = xxd.execute(invocation);
    TEST_ASSERT_EQUAL(0, first.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("00000000") != ETString::npos);

    stream.stdoutBuffer = "";
    stdinChannel.buffer = "n";
    CommandResult second = xxd.execute(invocation);
    TEST_ASSERT_EQUAL(0, second.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("00000010") != ETString::npos);
}

int process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_xxd_trigger_small_file);
    RUN_TEST(test_xxd_trigger_large_file);
    RUN_TEST(test_xxd_trigger_file_not_exists);
    RUN_TEST(test_xxd_usage);
    RUN_TEST(test_xxd_trigger_edge_cases);
    RUN_TEST(test_xxd_get_suggestions);
    RUN_TEST(test_xxd_execute_writes_stdout);
    RUN_TEST(test_xxd_execute_missing_file_writes_stderr);
    RUN_TEST(test_xxd_execute_navigates_on_next_key);
    UNITY_END();
    return 0;
}

#if (defined(ESP_PLATFORM) || defined(ESP32)) && not defined(ARDUINO)
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
    return process_tests();
}
#endif

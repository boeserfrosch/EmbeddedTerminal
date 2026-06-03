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
#include "../../../src/DirectoryNavigator.h"
#include "../utils.h"
#include "StorageSystem.h"
#include "../../Mocks/native/MockStorageMedia.h"

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
    auto iHandle = TestCommandInvocationHandle(keyword, {arg});
    CommandResult result = xxd.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains("68 65 6C 6C 6F 31 32 33 34")); // Hex for "hello123"
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
    auto iHandle = TestCommandInvocationHandle(keyword, {arg});

    CommandResult result = xxd.invoke(iHandle.invocation);
    TEST_ASSERT_EQUAL(CommandExecutionState::Running, result.state); // Should be running since we read 512 bytes and not truncate
    iHandle.input.print("n");                                        // Simulate user pressing 'n' for next chunk
    result = xxd.resume(iHandle.invocation);

    TEST_ASSERT_EQUAL(CommandExecutionState::Running, result.state);

    iHandle.input.print("q"); // Simulate user pressing 'q' to quit
    result = xxd.resume(iHandle.invocation);

    TEST_ASSERT_EQUAL(CommandExecutionState::Completed, result.state);

    TEST_ASSERT_TRUE(iHandle.output.contains("41 42 43")); // Hex for "ABC"
    TEST_ASSERT_TRUE(iHandle.output.length() > 1000);      // Should be large since we read 512 bytes and not truncate
    TEST_ASSERT_TRUE(iHandle.output.contains("0000020:"));
    TEST_ASSERT_FALSE(iHandle.output.contains("00000200:")); // The dump should stop before offset 200 for this data set
}

void test_xxd_trigger_file_not_exists(void)
{
    EmbeddedTerminal::cmd::xxd xxd(*dir);

    ETString keyword = "xxd";
    ETString arg = "nofile.txt";
    auto iHandle = TestCommandInvocationHandle(keyword, {arg});
    CommandResult result = xxd.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.error.contains("file not found"));
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
    auto iHandle = TestCommandInvocationHandle("xxd");
    CommandResult result = xxd.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains(xxd.usage("xxd")));

    // Test directory instead of file
    storage->mkdir("/dir");
    auto iHandle2 = TestCommandInvocationHandle("xxd", {"dir"});
    CommandResult result2 = xxd.invoke(iHandle2.invocation);
    TEST_ASSERT_TRUE(iHandle2.error.contains("path points to a directory"));
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
    EmptyInputChannel input;
    StreamBackedOutputChannel output(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel error(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{"xxd", {"file.txt"}, context, input, output, error};
    CommandResult result = xxd.invoke(invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("00000000") != ETString::npos);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("68 65 6C 6C 6F") != ETString::npos);
    TEST_ASSERT_TRUE(stream.stderrBuffer.empty());
}

void test_xxd_execute_missing_file_writes_stderr(void)
{
    EmbeddedTerminal::cmd::xxd xxd(*dir);

    auto iHandle = TestCommandInvocationHandle("xxd", {"nofile.txt"});
    CommandResult result = xxd.invoke(iHandle.invocation);

    TEST_ASSERT_EQUAL(EmbeddedTerminal::cmd::xxd::ErrorCode::FILE_NOT_FOUND, result.exitCode);
    TEST_ASSERT_TRUE(iHandle.error.contains("file not found"));
    TEST_ASSERT_TRUE(iHandle.output.empty());
}

void test_xxd_execute_navigates_on_next_key(void)
{
    ETString content = "";
    for (int i = 0; i < 30; i++)
    {
        content += "0123456789";
    }
    auto file = storage->open("/big.bin", "w", true);
    file.writeAll(content.c_str());
    file.close();

    EmbeddedTerminal::cmd::xxd xxd(*dir);
    MockStream stream;
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    BufferedInputChannel input;
    StreamBackedOutputChannel output(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel error(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{"xxd", {"big.bin"}, context, input, output, error};
    CommandResult first = xxd.invoke(invocation);
    TEST_ASSERT_EQUAL(0, first.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("00000000") != ETString::npos);

    stream.stdoutBuffer = "";
    input.buffer = "n";
    CommandResult second = xxd.invoke(invocation);
    TEST_ASSERT_EQUAL(0, second.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("00000010") != ETString::npos);
}

void test_xxd_execute_navigates_on_previous_key(void)
{
    ETString content = "";
    for (int i = 0; i < 40; i++)
    {
        content += "0123456789";
    }
    auto file = storage->open("/big_prev.bin", "w", true);
    file.writeAll(content.c_str());
    file.close();

    EmbeddedTerminal::cmd::xxd xxd(*dir);
    MockStream stream;
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    BufferedInputChannel input;
    StreamBackedOutputChannel output(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel error(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{"xxd", {"big_prev.bin"}, context, input, output, error};
    xxd.invoke(invocation);

    input.buffer = "n";
    xxd.invoke(invocation);

    stream.stdoutBuffer = "";
    input.buffer = "p";
    CommandResult result = xxd.invoke(invocation);
    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("00000000") != ETString::npos);
}

void test_xxd_execute_navigates_with_go_begin_and_end(void)
{
    ETString content = "";
    for (int i = 0; i < 120; i++)
    {
        content += "ABCDEFGH";
    }
    storage->open("/big_go.bin", "w", true).writeAll(content.c_str());

    EmbeddedTerminal::cmd::xxd xxd(*dir);
    MockStream stream;
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    BufferedInputChannel input;
    StreamBackedOutputChannel output(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel error(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{"xxd", {"big_go.bin"}, context, input, output, error};
    xxd.invoke(invocation);

    stream.stdoutBuffer = "";
    input.buffer = "G";
    xxd.resume(invocation);
    TEST_ASSERT_TRUE(vars.find("xxd__pos") != vars.end());
    TEST_ASSERT_TRUE(std::stoull(vars["xxd__pos"].c_str()) > 0);

    stream.stdoutBuffer = "";
    input.buffer = "g";
    CommandResult result = xxd.resume(invocation);
    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("00000000") != ETString::npos);
}

void test_xxd_execute_navigates_to_hex_offset(void)
{
    ETString content = "";
    for (int i = 0; i < 800; i++)
    {
        content += "0123456789";
    }
    storage->open("/big_offset.bin", "w", true).writeAll(content.c_str());

    EmbeddedTerminal::cmd::xxd xxd(*dir);
    MockStream stream;
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    BufferedInputChannel input;
    StreamBackedOutputChannel output(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel error(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{"xxd", {"big_offset.bin"}, context, input, output, error};
    xxd.invoke(invocation);

    stream.stdoutBuffer = "";
    input.buffer = "o200\n";
    CommandResult result = xxd.resume(invocation);
    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("00000200") != ETString::npos);
}

void test_xxd_execute_navigates_to_hex_offset_in_chunks(void)
{
    ETString content = "";
    for (int i = 0; i < 800; i++)
    {
        content += "0123456789";
    }
    storage->open("/big_offset_chunked.bin", "w", true).writeAll(content.c_str());

    EmbeddedTerminal::cmd::xxd xxd(*dir);
    MockStream stream;
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    BufferedInputChannel input;
    StreamBackedOutputChannel output(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel error(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{"xxd", {"big_offset_chunked.bin"}, context, input, output, error};
    xxd.invoke(invocation);

    ETString initialPos = vars["xxd__pos"];
    stream.stdoutBuffer = "";
    input.buffer = "o";
    CommandResult partial = xxd.resume(invocation);
    TEST_ASSERT_EQUAL(CommandExecutionState::Running, partial.state);
    TEST_ASSERT_EQUAL_STRING(initialPos.c_str(), vars["xxd__pos"].c_str());
    TEST_ASSERT_TRUE(stream.stdoutBuffer.empty());

    stream.stdoutBuffer = "";
    input.buffer = "200\n";
    CommandResult result = xxd.resume(invocation);
    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("00000200") != ETString::npos);
}

void test_xxd_execute_quit_key_completes(void)
{
    storage->open("/quit.bin", "w", true).writeAll("0123456789ABCDEF");
    EmbeddedTerminal::cmd::xxd xxd(*dir);

    auto iHandle = TestCommandInvocationHandle("xxd", {"quit.bin"});
    xxd.invoke(iHandle.invocation);

    iHandle.input.print("q");
    CommandResult result = xxd.resume(iHandle.invocation);
    TEST_ASSERT_EQUAL(CommandExecutionState::Completed, result.state);
    TEST_ASSERT_EQUAL(0, result.exitCode);
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
    RUN_TEST(test_xxd_execute_navigates_on_previous_key);
    RUN_TEST(test_xxd_execute_navigates_with_go_begin_and_end);
    RUN_TEST(test_xxd_execute_navigates_to_hex_offset);
    RUN_TEST(test_xxd_execute_navigates_to_hex_offset_in_chunks);
    RUN_TEST(test_xxd_execute_quit_key_completes);
    UNITY_END();
    return 0;
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
    return process_tests();
}
#endif

// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif

#include "commands/wc.h"
#include "StorageSystem.h"
#include "../Mocks/native/MockFileSystem.h"
#include "../Mocks/native/MockStorageMedia.h"
#include "../Mocks/MockStream.h"
#include "../Mocks/CommandRuntimeTestUtils.h"

using namespace EmbeddedTerminal;

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

void test_wc_trigger_file_counts(void)
{
    ETFile file = storage->open("/sample.txt", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(file.isOpen());
    TEST_ASSERT_TRUE(file.writeAll("one two\nthree\n"));
    file.close();

    cmd::wc wcCmd(*dir);
    ETString result = wcCmd.trigger("wc", "sample.txt");

    TEST_ASSERT_EQUAL_STRING("2 3 14 sample.txt\n", result.c_str());
}

void test_wc_trigger_missing_file(void)
{
    cmd::wc wcCmd(*dir);
    ETString result = wcCmd.trigger("wc", "missing.txt");

    TEST_ASSERT_TRUE(result.find("did not exist") != ETString::npos);
}

void test_wc_usage(void)
{
    cmd::wc wcCmd(*dir);
    ETString usage = wcCmd.usage("wc");
    TEST_ASSERT_TRUE(usage.find("line, word and byte") != ETString::npos);
}

void test_wc_execute_reads_stdin_when_no_file(void)
{
    cmd::wc wcCmd(*dir);

    MockStream stream;
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    BufferedInputChannel stdinChannel;
    stdinChannel.buffer = "a b\nc\n";
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{"wc", "", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = wcCmd.execute(invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_EQUAL(CommandExecutionState::Completed, result.state);
    TEST_ASSERT_EQUAL_STRING("2 3 6\n", stream.stdoutBuffer.c_str());
    TEST_ASSERT_TRUE(stream.stderrBuffer.empty());
}

void test_wc_execute_file_counts(void)
{
    ETFile file = storage->open("/file.txt", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(file.isOpen());
    TEST_ASSERT_TRUE(file.writeAll("hello world\n"));
    file.close();

    cmd::wc wcCmd(*dir);

    MockStream stream;
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{"wc", "file.txt", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = wcCmd.execute(invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_EQUAL(CommandExecutionState::Completed, result.state);
    TEST_ASSERT_EQUAL_STRING("1 2 12 file.txt\n", stream.stdoutBuffer.c_str());
    TEST_ASSERT_TRUE(stream.stderrBuffer.empty());
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_wc_trigger_file_counts);
    RUN_TEST(test_wc_trigger_missing_file);
    RUN_TEST(test_wc_usage);
    RUN_TEST(test_wc_execute_reads_stdin_when_no_file);
    RUN_TEST(test_wc_execute_file_counts);
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

// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif

#include "commands/touch.h"
#include "../../Mocks/native/MockFileSystem.h"
#include "../../Mocks/MockStream.h"
#include "../../Mocks/CommandRuntimeTestUtils.h"
#include "StorageSystem.h"
#include "../../Mocks/native/MockStorageMedia.h"

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

void test_touch_creates_missing_file(void)
{
    cmd::touch touchCmd(*dir);
    ETString result = touchCmd.trigger("touch", "new.txt");

    TEST_ASSERT_TRUE(result.find("touched") != ETString::npos);
    TEST_ASSERT_TRUE(storage->exists("/new.txt"));
}

void test_touch_existing_file(void)
{
    storage->open("/existing.txt", FILE_MODE_WRITE, true).close();

    cmd::touch touchCmd(*dir);
    ETString result = touchCmd.trigger("touch", "existing.txt");

    TEST_ASSERT_TRUE(result.find("touched") != ETString::npos);
    TEST_ASSERT_TRUE(storage->exists("/existing.txt"));
}

void test_touch_existing_file_keeps_content(void)
{
    auto file = storage->open("/content.txt", FILE_MODE_WRITE, true);
    file.writeAll("abc123");
    file.close();

    cmd::touch touchCmd(*dir);
    ETString result = touchCmd.trigger("touch", "content.txt");

    TEST_ASSERT_TRUE(result.find("touched") != ETString::npos);
    auto readFile = storage->open("/content.txt", FILE_MODE_READ, false);
    TEST_ASSERT_EQUAL_STRING("abc123", readFile.readAll().c_str());
    readFile.close();
}

void test_touch_rejects_directory(void)
{
    storage->mkdir("/dir");

    cmd::touch touchCmd(*dir);
    ETString result = touchCmd.trigger("touch", "dir");

    TEST_ASSERT_TRUE(result.find("directory") != ETString::npos);
}

void test_touch_empty_path(void)
{
    cmd::touch touchCmd(*dir);
    ETString result = touchCmd.trigger("touch", "   ");
    TEST_ASSERT_TRUE(result.find("path or name to file expected") != ETString::npos);
}

void test_touch_usage(void)
{
    cmd::touch touchCmd(*dir);
    ETString usage = touchCmd.usage("touch");
    TEST_ASSERT_TRUE(usage.find("leave existing file unchanged") != ETString::npos);
}

void test_touch_execute_writes_stdout(void)
{
    cmd::touch touchCmd(*dir);

    MockStream stream;
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{"touch", "runtime.txt", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = touchCmd.execute(invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("touched") != ETString::npos);
    TEST_ASSERT_TRUE(stream.stderrBuffer.empty());
    TEST_ASSERT_TRUE(storage->exists("/runtime.txt"));
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_touch_creates_missing_file);
    RUN_TEST(test_touch_existing_file);
    RUN_TEST(test_touch_existing_file_keeps_content);
    RUN_TEST(test_touch_rejects_directory);
    RUN_TEST(test_touch_empty_path);
    RUN_TEST(test_touch_usage);
    RUN_TEST(test_touch_execute_writes_stdout);
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

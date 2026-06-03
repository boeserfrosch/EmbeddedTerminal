#include <unity.h>

#include "../../../src/channels/BufferedInput.h"
#include "../../../src/channels/BufferedOutput.h"
#include "../../../src/channels/BufferedInOut.h"
#include "../../../src/channels/FileInput.h"
#include "../../../src/channels/FileOutput.h"
#include "../../Mocks/MockStream.h"
#include "../../Mocks/native/MockFile.h"
#include "../../../src/ETFile.h"

using namespace EmbeddedTerminal;
using namespace EmbeddedTerminal::Channels;

void setUp(void) {}
void tearDown(void) {}

// ===== BufferedInput Tests =====

void test_buffered_input_basic(void)
{
    BufferedInput in("hello");
    TEST_ASSERT_TRUE(in.available());
    ETString all = in.readAll();
    TEST_ASSERT_EQUAL_STRING("hello", all.c_str());
    TEST_ASSERT_FALSE(in.available());
    ETString empty = in.readAll();
    TEST_ASSERT_EQUAL_STRING("", empty.c_str());
}

void test_buffered_input_empty_content(void)
{
    BufferedInput in("");
    TEST_ASSERT_FALSE(in.available());
    ETString result = in.readAll();
    TEST_ASSERT_EQUAL_STRING("", result.c_str());
}

void test_buffered_input_chunk_boundaries(void)
{
    ETString content = "line1\nline2\nline3";
    BufferedInput in(content);

    TEST_ASSERT_TRUE(in.available());
    ETString chunk1 = in.readAll();
    TEST_ASSERT_EQUAL_STRING("line1\nline2\nline3", chunk1.c_str());

    TEST_ASSERT_FALSE(in.available());
    ETString chunk2 = in.readAll();
    TEST_ASSERT_EQUAL_STRING("", chunk2.c_str());
}

void test_buffered_input_large_content(void)
{
    ETString large("a");
    for (int i = 0; i < 100; i++)
    {
        large += "b";
    }

    BufferedInput in(large);
    TEST_ASSERT_TRUE(in.available());
    ETString result = in.readAll();
    TEST_ASSERT_EQUAL(101, result.length());
    TEST_ASSERT_FALSE(in.available());
}

// ===== BufferedOutput Tests =====

void test_buffered_output_accumulate(void)
{
    BufferedOutput out;
    out.print("a");
    out.print("bc");
    TEST_ASSERT_EQUAL_STRING("abc", out.getBuffer().c_str());
}

void test_buffered_output_empty(void)
{
    BufferedOutput out;
    TEST_ASSERT_EQUAL_STRING("", out.getBuffer().c_str());
}

void test_buffered_output_multiple_writes(void)
{
    BufferedOutput out;
    for (int i = 0; i < 5; i++)
    {
        out.print("x");
    }
    TEST_ASSERT_EQUAL_STRING("xxxxx", out.getBuffer().c_str());
}

void test_buffered_output_clear_behavior(void)
{
    BufferedOutput out;
    out.print("first");
    const ETString &buf1 = out.getBuffer();
    TEST_ASSERT_EQUAL_STRING("first", buf1.c_str());

    out.print(" second");
    const ETString &buf2 = out.getBuffer();
    TEST_ASSERT_EQUAL_STRING("first second", buf2.c_str());
}

// ===== BufferedInOut Tests =====

void test_buffered_inout_write_then_read(void)
{
    BufferedInOut io;
    io.print("hello");
    io.print(" world");

    TEST_ASSERT_TRUE(io.available());
    ETString data = io.readAll();
    TEST_ASSERT_EQUAL_STRING("hello world", data.c_str());

    TEST_ASSERT_FALSE(io.available());
}

void test_buffered_inout_multiple_write_read_sequences(void)
{
    BufferedInOut io;

    // First sequence
    io.print("first");
    ETString result1 = io.readAll();
    TEST_ASSERT_EQUAL_STRING("first", result1.c_str());
    TEST_ASSERT_FALSE(io.available());

    // Second sequence (writes after reading continue from read position)
    io.print("second");
    ETString result2 = io.readAll();
    // After first read, position is at 5. New data is appended at end.
    // ReadAll from position 5 returns "second"
    TEST_ASSERT_EQUAL_STRING("second", result2.c_str());
}

void test_buffered_inout_interleaved_write_read(void)
{
    BufferedInOut io;

    io.print("a");
    io.print("b");
    TEST_ASSERT_TRUE(io.available());

    ETString all = io.readAll();
    TEST_ASSERT_EQUAL_STRING("ab", all.c_str());

    io.print("c");
    io.print("d");

    // After reading "ab", position is at 2. New writes are appended.
    // Next readAll from position 2 returns "cd"
    ETString more = io.readAll();
    TEST_ASSERT_EQUAL_STRING("cd", more.c_str());
}

void test_buffered_inout_available_tracking(void)
{
    BufferedInOut io;

    TEST_ASSERT_FALSE(io.available());
    io.print("data");
    TEST_ASSERT_TRUE(io.available());

    io.readAll();
    TEST_ASSERT_FALSE(io.available());

    io.print("more");
    TEST_ASSERT_TRUE(io.available());
}

// ===== FileInput Tests =====

void test_file_input_basic_read(void)
{
    auto mockFile = std::make_shared<MockFile>("hello world");
    ETFile file(mockFile);
    FileInput reader(file);

    TEST_ASSERT_TRUE(reader.available());
    ETString content = reader.readAll();
    TEST_ASSERT_EQUAL_STRING("hello world", content.c_str());

    // Note: MockFile's readAll() doesn't advance position, so available() still returns true
    // This matches the expected behavior where readAll() returns the full content from current position
    TEST_ASSERT_TRUE(reader.available());
}

void test_file_input_empty_file(void)
{
    auto mockFile = std::make_shared<MockFile>("");
    ETFile file(mockFile);
    FileInput reader(file);

    TEST_ASSERT_FALSE(reader.available());
    ETString content = reader.readAll();
    TEST_ASSERT_EQUAL_STRING("", content.c_str());
}

void test_file_input_large_file(void)
{
    ETString largeContent;
    for (int i = 0; i < 1000; i++)
    {
        largeContent += "line ";
    }

    auto mockFile = std::make_shared<MockFile>(largeContent);
    ETFile file(mockFile);
    FileInput reader(file);

    TEST_ASSERT_TRUE(reader.available());
    ETString content = reader.readAll();
    TEST_ASSERT_EQUAL(largeContent.length(), content.length());
    TEST_ASSERT_EQUAL_STRING(largeContent.c_str(), content.c_str());
}

void test_file_input_multiline_content(void)
{
    ETString content = "line1\nline2\nline3\n";
    auto mockFile = std::make_shared<MockFile>(content);
    ETFile file(mockFile);
    FileInput reader(file);

    TEST_ASSERT_TRUE(reader.available());
    ETString result = reader.readAll();
    TEST_ASSERT_EQUAL_STRING(content.c_str(), result.c_str());
}

// ===== FileOutput Tests =====

void test_file_output_basic_write(void)
{
    auto mockFile = std::make_shared<MockFile>();
    mockFile->open_ = true;
    ETFile file(mockFile);
    FileOutput writer(file);

    writer.print("hello");
    writer.print(" ");
    writer.print("world");

    TEST_ASSERT_EQUAL_STRING("hello world", mockFile->content_.c_str());
}

void test_file_output_empty_write(void)
{
    auto mockFile = std::make_shared<MockFile>();
    mockFile->open_ = true;
    ETFile file(mockFile);
    FileOutput writer(file);

    writer.print("");
    TEST_ASSERT_EQUAL_STRING("", mockFile->content_.c_str());
}

void test_file_output_multiple_writes(void)
{
    auto mockFile = std::make_shared<MockFile>();
    mockFile->open_ = true;
    ETFile file(mockFile);
    FileOutput writer(file);

    for (int i = 0; i < 5; i++)
    {
        writer.print("x");
    }

    TEST_ASSERT_EQUAL_STRING("xxxxx", mockFile->content_.c_str());
}

void test_file_output_special_characters(void)
{
    auto mockFile = std::make_shared<MockFile>();
    mockFile->open_ = true;
    ETFile file(mockFile);
    FileOutput writer(file);

    writer.print("hello\nworld\t!");
    TEST_ASSERT_EQUAL_STRING("hello\nworld\t!", mockFile->content_.c_str());
}

// ===== FileInput/FileOutput Integration Tests =====

void test_file_io_write_then_read(void)
{
    auto sharedContent = std::make_shared<ETString>();
    auto writeFile = std::make_shared<MockFile>(sharedContent, true);
    auto readFile = std::make_shared<MockFile>(sharedContent, true);

    ETFile out(writeFile);
    ETFile in(readFile);

    FileOutput writer(out);
    FileInput reader(in);

    writer.print("test data");

    TEST_ASSERT_TRUE(reader.available());
    ETString result = reader.readAll();
    TEST_ASSERT_EQUAL_STRING("test data", result.c_str());
}

int process_tests()
{
    UNITY_BEGIN();

    // BufferedInput tests
    RUN_TEST(test_buffered_input_basic);
    RUN_TEST(test_buffered_input_empty_content);
    RUN_TEST(test_buffered_input_chunk_boundaries);
    RUN_TEST(test_buffered_input_large_content);

    // BufferedOutput tests
    RUN_TEST(test_buffered_output_accumulate);
    RUN_TEST(test_buffered_output_empty);
    RUN_TEST(test_buffered_output_multiple_writes);
    RUN_TEST(test_buffered_output_clear_behavior);

    // BufferedInOut tests
    RUN_TEST(test_buffered_inout_write_then_read);
    RUN_TEST(test_buffered_inout_multiple_write_read_sequences);
    RUN_TEST(test_buffered_inout_interleaved_write_read);
    RUN_TEST(test_buffered_inout_available_tracking);

    // FileInput tests
    RUN_TEST(test_file_input_basic_read);
    RUN_TEST(test_file_input_empty_file);
    RUN_TEST(test_file_input_large_file);
    RUN_TEST(test_file_input_multiline_content);

    // FileOutput tests
    RUN_TEST(test_file_output_basic_write);
    RUN_TEST(test_file_output_empty_write);
    RUN_TEST(test_file_output_multiple_writes);
    RUN_TEST(test_file_output_special_characters);

    // FileInput/FileOutput integration
    RUN_TEST(test_file_io_write_then_read);

    UNITY_END();
    return 0;
}

#if (defined(ESP_PLATFORM) || defined(ESP32)) && !defined(ARDUINO)
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
// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <unity.h>

#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif

#include <algorithm>

#include "hal/native/NativeFile.h"
#include "hal/native/NativeFileSystem.h"

using namespace EmbeddedTerminal;

static const char *TEST_DIR = "testdir";
static const char *TEST_FILE = "testdir/test.txt";
static const char *TEST_FILE2 = "testdir/test2.txt";

void setUp(void)
{
    // Clean up before each test
    std::filesystem::remove_all(TEST_DIR);
    std::filesystem::create_directories(TEST_DIR);
}

void tearDown(void)
{
    // Clean up after each test
    std::filesystem::remove_all(TEST_DIR);
}

void test_file_write_and_read(void)
{
    NativeFileSystem fs;
    auto file = fs.open(TEST_FILE, FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(file.isOpen());

    ETString data = "HelloWorld";
    TEST_ASSERT_TRUE(file.writeAll(data));
    file.close();

    auto file2 = fs.open(TEST_FILE, FILE_MODE_READ);
    TEST_ASSERT_TRUE(file2.isOpen());
    ETString content = file2.readAll();
    TEST_ASSERT_EQUAL_STRING(data.c_str(), content.c_str());
}

void test_file_append(void)
{
    NativeFileSystem fs;
    {
        auto file = fs.open(TEST_FILE, FILE_MODE_WRITE, true);
        file.writeAll("ABC");
    }
    {
        auto file = fs.open(TEST_FILE, FILE_MODE_APPEND);
        file.write(reinterpret_cast<const void *>("DEF"), 3);
    }
    auto file2 = fs.open(TEST_FILE, FILE_MODE_READ);
    ETString content = file2.readAll();
    TEST_ASSERT_EQUAL_STRING("ABCDEF", content.c_str());
}

void test_file_seek_and_position(void)
{
    NativeFileSystem fs;
    auto file = fs.open(TEST_FILE, FILE_MODE_WRITE, true);
    file.writeAll("1234567890");
    file.seek(5);
    TEST_ASSERT_EQUAL(5, file.position());
    auto x = ETString("X").c_str();
    file.write(x, 1);
    file.close();

    auto file2 = fs.open(TEST_FILE, FILE_MODE_READ);
    ETString content = file2.readAll();
    TEST_ASSERT_EQUAL_STRING("12345X7890", content.c_str());
}

void test_filesystem_exists_and_remove(void)
{
    NativeFileSystem fs;
    auto file = fs.open(TEST_FILE, FILE_MODE_WRITE, true);
    file.writeAll("temp");
    file.close();

    TEST_ASSERT_TRUE(fs.exists(TEST_FILE));
    TEST_ASSERT_TRUE(fs.remove(TEST_FILE));
    TEST_ASSERT_FALSE(fs.exists(TEST_FILE));
}

void test_filesystem_directory(void)
{
    NativeFileSystem fs;
    TEST_ASSERT_TRUE(fs.isDirectory(TEST_DIR));
    TEST_ASSERT_TRUE(fs.mkdir("testdir/subdir"));
    TEST_ASSERT_TRUE(fs.isDirectory("testdir/subdir"));
    TEST_ASSERT_TRUE(fs.rmdir("testdir/subdir"));
    TEST_ASSERT_FALSE(fs.exists("testdir/subdir"));
}

void test_filesystem_list(void)
{
    NativeFileSystem fs;
    {
        auto f1 = fs.open(TEST_FILE, FILE_MODE_WRITE, true);
        f1.writeAll("file1");
    }
    {
        auto f2 = fs.open(TEST_FILE2, FILE_MODE_WRITE, true);
        f2.writeAll("file2");
    }

    auto list = fs.list(TEST_DIR);
    TEST_ASSERT_EQUAL(2, list.size());

    bool found1 = std::find(list.begin(), list.end(), Path("test.txt")) != list.end();
    bool found2 = std::find(list.begin(), list.end(), Path("test2.txt")) != list.end();
    TEST_ASSERT_TRUE(found1 && found2);
}

int process_tests_filesystem()
{
    UNITY_BEGIN();
    RUN_TEST(test_file_write_and_read);
    RUN_TEST(test_file_append);
    RUN_TEST(test_file_seek_and_position);
    RUN_TEST(test_filesystem_exists_and_remove);
    RUN_TEST(test_filesystem_directory);
    RUN_TEST(test_filesystem_list);
    UNITY_END();
    return 0;
}

#if (defined(ESP_PLATFORM) || defined(ESP32)) && !defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
extern "C" void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(4000));
    process_tests_filesystem();
}
#elif defined(ARDUINO)
void setup()
{
    delay(2500);
    process_tests_filesystem();
}
void loop() {}
#else
int main()
{
    return process_tests_filesystem();
}
#endif

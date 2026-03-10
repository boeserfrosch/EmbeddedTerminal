#include <unity.h>
#include "../../Mocks/native/MockFileSystem.h"

using namespace EmbeddedTerminal;

MockFileSystem *fs = nullptr;

void setUp(void)
{
    fs = new MockFileSystem();
}

void tearDown(void)
{
    delete fs;
    fs = nullptr;
}

void test_mkdir_and_exists()
{
    TEST_ASSERT_TRUE(fs->mkdir("/dir"));
    TEST_ASSERT_TRUE(fs->exists("/dir"));
    TEST_ASSERT_FALSE(fs->mkdir("/dir")); // Already exists
    TEST_ASSERT_TRUE(fs->isDirectory("/dir"));
}

void test_open_and_write_read()
{
    auto file = fs->open("/file.txt", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(file.isOpen());
    const char *data = "hello";
    TEST_ASSERT_EQUAL_size_t(5, file.write(data, 5));
    TEST_ASSERT_TRUE(file.seek(0));
    char buf[6] = {0};
    TEST_ASSERT_EQUAL_size_t(5, file.read(buf, 5));
    TEST_ASSERT_EQUAL_STRING("hello", buf);
}

void test_remove_file()
{
    fs->open("/file.txt", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(fs->exists("/file.txt"));
    TEST_ASSERT_TRUE(fs->remove("/file.txt"));
    TEST_ASSERT_FALSE(fs->exists("/file.txt"));
    TEST_ASSERT_FALSE(fs->remove("/file.txt")); // Already removed
}

void test_rmdir()
{
    fs->mkdir("/dir");
    TEST_ASSERT_TRUE(fs->exists("/dir"));
    TEST_ASSERT_TRUE(fs->rmdir("/dir"));
    TEST_ASSERT_FALSE(fs->exists("/dir"));
    TEST_ASSERT_FALSE(fs->rmdir("/dir")); // Already removed
}

void test_list_directory()
{
    fs->mkdir("/dir");
    fs->open("/dir/file1.txt", FILE_MODE_WRITE, true);
    fs->open("/dir/file2.txt", FILE_MODE_WRITE, true);
    auto list = fs->list("/dir");

    TEST_ASSERT_EQUAL(2, list.size()); // file1.txt, file2.txt
    // Test the formatting of the return ed paths
    TEST_ASSERT_EQUAL_STRING("file1.txt", list[0].c_str());
    TEST_ASSERT_EQUAL_STRING("file2.txt", list[1].c_str());
}

void test_is_empty()
{
    fs->mkdir("/dir");
    TEST_ASSERT_TRUE(fs->isEmpty("/dir"));
    fs->open("/dir/file.txt", FILE_MODE_WRITE, true);
    TEST_ASSERT_FALSE(fs->isEmpty("/dir"));
}

void test_is_directory()
{
    fs->mkdir("/dir");
    fs->open("/file.txt", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(fs->isDirectory("/dir"));
    TEST_ASSERT_FALSE(fs->isDirectory("/file.txt"));
    TEST_ASSERT_FALSE(fs->isDirectory("/notfound"));
}

void test_open_nonexistent_file_no_create()
{
    auto file = fs->open("/nofile.txt", FILE_MODE_READ, false);
    TEST_ASSERT_FALSE(file.isOpen());
}

void test_open_nonexistent_file_with_create()
{
    auto file = fs->open("/newfile.txt", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(file.isOpen());
    TEST_ASSERT_TRUE(fs->exists("/newfile.txt"));
}

void test_directory_as_file()
{
    fs->mkdir("/adir");
    auto file = fs->open("/adir", FILE_MODE_READ, false);
    TEST_ASSERT_FALSE(file.isOpen()); // Should not open a directory as a file
}

void test_root_directory_exists()
{
    TEST_ASSERT_TRUE(fs->exists("/"));
    TEST_ASSERT_TRUE(fs->isDirectory("/"));
    TEST_ASSERT_TRUE(fs->isEmpty("/"));
}

void test_file_position_after_reopen()
{
    auto file = fs->open("/pos.txt", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(file.isOpen());
    file.write("abc", 3);
    TEST_ASSERT_EQUAL(3, file.position());
    file.close();

    // Reopen for reading, position should be 0
    auto file2 = fs->open("/pos.txt", FILE_MODE_READ, false);
    TEST_ASSERT_TRUE(file2.isOpen());
    TEST_ASSERT_EQUAL(0, file2.position());
    char buf[4] = {0};
    file2.read(buf, 3);
    TEST_ASSERT_EQUAL_STRING("abc", buf);
    TEST_ASSERT_EQUAL(3, file2.position());
}

void test_open_file_multiple_times()
{
    auto file1 = fs->open("/multi.txt", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(file1.isOpen());
    file1.write("data", 4);

    // Try to open the same file again for reading
    auto file2 = fs->open("/multi.txt", FILE_MODE_READ, false);
    TEST_ASSERT_TRUE(file2.isOpen());
    char buf[5] = {0};
    file2.read(buf, 4);
    TEST_ASSERT_EQUAL_STRING("data", buf);

    // Try to open the same file again for writing - should be blocked since file1 is still open for writing
    auto nonfile3 = fs->open("/multi.txt", FILE_MODE_WRITE, false);
    TEST_ASSERT_FALSE(nonfile3.isOpen());
    file1.close();

    // After closing, should be able to open for write again (write mode truncates)
    auto file3 = fs->open("/multi.txt", FILE_MODE_WRITE, false);
    TEST_ASSERT_TRUE(file3.isOpen());
    file3.write("X", 1);
    TEST_ASSERT_EQUAL(1, file3.size());
    TEST_ASSERT_EQUAL_STRING("X", file3.readAll().c_str());
}
void test_multiple_read_handles()
{
    fs->open("/multi.txt", FILE_MODE_WRITE, true).writeAll("data");
    auto r1 = fs->open("/multi.txt", FILE_MODE_READ, false);
    auto r2 = fs->open("/multi.txt", FILE_MODE_READ, false);
    TEST_ASSERT_TRUE(r1.isOpen());
    TEST_ASSERT_TRUE(r2.isOpen());
    char buf1[5] = {0};
    char buf2[5] = {0};
    r1.read(buf1, 4);
    r2.read(buf2, 4);
    TEST_ASSERT_EQUAL_STRING("data", buf1);
    TEST_ASSERT_EQUAL_STRING("data", buf2);
}

void test_single_write_handle()
{
    auto w1 = fs->open("/single.txt", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(w1.isOpen());
    // Try to open another write handle
    auto w2 = fs->open("/single.txt", FILE_MODE_WRITE, false);
    TEST_ASSERT_FALSE(w2.isOpen());
    w1.close();
    // After closing, should be able to open for write again
    auto w3 = fs->open("/single.txt", FILE_MODE_WRITE, false);
    TEST_ASSERT_TRUE(w3.isOpen());
}

void test_write_with_read_handles_open()
{
    fs->open("/rw.txt", FILE_MODE_WRITE, true).writeAll("abc");
    auto r1 = fs->open("/rw.txt", FILE_MODE_READ, false);
    auto w1 = fs->open("/rw.txt", FILE_MODE_WRITE, false);
    TEST_ASSERT_TRUE(w1.isOpen());
    w1.writeAll("d");
    w1.close();
    r1.close();
}

void test_distinct_handles()
{
    fs->open("/distinct.txt", FILE_MODE_WRITE, true).writeAll("xyz");
    auto h1 = fs->open("/distinct.txt", FILE_MODE_READ, false);
    auto h2 = fs->open("/distinct.txt", FILE_MODE_READ, false);
    TEST_ASSERT_TRUE(h1.isOpen());
    TEST_ASSERT_TRUE(h2.isOpen());
    TEST_ASSERT_NOT_EQUAL(&h1, &h2); // Should be distinct objects
}

void test_recursive_mkdir_creates_parents()
{
    TEST_ASSERT_TRUE(fs->mkdir("foo/bar/baz"));
    TEST_ASSERT_TRUE(fs->exists("/foo"));
    TEST_ASSERT_TRUE(fs->exists("/foo/bar"));
    TEST_ASSERT_TRUE(fs->exists("/foo/bar/baz"));
    TEST_ASSERT_TRUE(fs->isDirectory("/foo"));
    TEST_ASSERT_TRUE(fs->isDirectory("/foo/bar"));
    TEST_ASSERT_TRUE(fs->isDirectory("/foo/bar/baz"));
}

void test_open_create_creates_missing_parent_directories()
{
    auto file = fs->open("/dir3/foo.txt", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(file.isOpen());
    TEST_ASSERT_TRUE(fs->exists("/dir3"));
    TEST_ASSERT_TRUE(fs->isDirectory("/dir3"));
    TEST_ASSERT_TRUE(fs->exists("/dir3/foo.txt"));
}

void test_relative_and_absolute_paths_are_equivalent()
{
    fs->open("/big.txt", FILE_MODE_WRITE, true).writeAll("data");
    TEST_ASSERT_TRUE(fs->exists("/big.txt"));
    TEST_ASSERT_TRUE(fs->exists("big.txt"));
}

int process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_mkdir_and_exists);
    RUN_TEST(test_open_and_write_read);
    RUN_TEST(test_remove_file);
    RUN_TEST(test_rmdir);
    RUN_TEST(test_list_directory);
    RUN_TEST(test_is_empty);
    RUN_TEST(test_is_directory);
    RUN_TEST(test_open_nonexistent_file_no_create);
    RUN_TEST(test_open_nonexistent_file_with_create);
    RUN_TEST(test_directory_as_file);
    RUN_TEST(test_root_directory_exists);
    RUN_TEST(test_file_position_after_reopen);
    RUN_TEST(test_open_file_multiple_times);
    RUN_TEST(test_multiple_read_handles);
    RUN_TEST(test_single_write_handle);
    RUN_TEST(test_write_with_read_handles_open);
    RUN_TEST(test_distinct_handles);
    RUN_TEST(test_recursive_mkdir_creates_parents);
    RUN_TEST(test_open_create_creates_missing_parent_directories);
    RUN_TEST(test_relative_and_absolute_paths_are_equivalent);
    return UNITY_END();
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

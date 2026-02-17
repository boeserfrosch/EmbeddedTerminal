// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif

#include <unity.h>
#include "../src/DirectoryNavigator.h"
#include "../Mocks/MockFileSystem.h"

using namespace EmbeddedTerminal;

void setUp(void) {}
void tearDown(void) {}

// Test basic construction
void test_navigator_construction(void)
{
    MockFileSystem fs;
    DirectoryNavigator nav(&fs);
    TEST_ASSERT_EQUAL_STRING("/", nav.pwd().c_str());
}

// Test construction with custom root
void test_navigator_custom_root(void)
{
    MockFileSystem fs;
    fs.createDirectory("/home");
    DirectoryNavigator nav(&fs, "/home");
    TEST_ASSERT_EQUAL_STRING("/home", nav.pwd().c_str());
}

// Test cd to absolute path
void test_navigator_cd_absolute(void)
{
    MockFileSystem fs;
    fs.createDirectory("/home");
    fs.createDirectory("/home/user");
    DirectoryNavigator nav(&fs);

    bool result = nav.cd("/home/user");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("/home/user", nav.pwd().c_str());
}

// Test cd to relative path
void test_navigator_cd_relative(void)
{
    MockFileSystem fs;
    fs.createDirectory("/home");
    fs.createDirectory("/home/user");
    DirectoryNavigator nav(&fs, "/home");

    bool result = nav.cd("user");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("/home/user", nav.pwd().c_str());
}

// Test cd to parent directory
void test_navigator_cd_parent(void)
{
    MockFileSystem fs;
    fs.createDirectory("/home");
    fs.createDirectory("/home/user");
    DirectoryNavigator nav(&fs, "/home/user");

    bool result = nav.cd("..");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("/home", nav.pwd().c_str());
}

// Test cd to non-existent directory
void test_navigator_cd_nonexistent(void)
{
    MockFileSystem fs;
    DirectoryNavigator nav(&fs);

    bool result = nav.cd("/nonexistent");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_STRING("/", nav.pwd().c_str()); // Should not change
}

// Test cd to file (should fail)
void test_navigator_cd_to_file(void)
{
    MockFileSystem fs;
    fs.createFile("/file.txt", "content", 7);
    DirectoryNavigator nav(&fs);

    bool result = nav.cd("/file.txt");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_STRING("/", nav.pwd().c_str());
}

// Test exists with absolute path
void test_navigator_exists_absolute(void)
{
    MockFileSystem fs;
    fs.createFile("/file.txt", "data", 4);
    DirectoryNavigator nav(&fs);

    TEST_ASSERT_TRUE(nav.exists("/file.txt"));
    TEST_ASSERT_FALSE(nav.exists("/missing.txt"));
}

// Test exists with relative path
void test_navigator_exists_relative(void)
{
    MockFileSystem fs;
    fs.createDirectory("/home");
    fs.createFile("/home/file.txt", "data", 4);
    DirectoryNavigator nav(&fs, "/home");

    TEST_ASSERT_TRUE(nav.exists("file.txt"));
    TEST_ASSERT_FALSE(nav.exists("missing.txt"));
}

// Test ls current directory
void test_navigator_ls_current(void)
{
    MockFileSystem fs;
    fs.createDirectory("/home");
    fs.createFile("/home/a.txt", "a", 1);
    fs.createFile("/home/b.txt", "b", 1);
    DirectoryNavigator nav(&fs, "/home");

    auto files = nav.ls();
    TEST_ASSERT_EQUAL(2, files.size());
}

// Test ls with path
void test_navigator_ls_path(void)
{
    MockFileSystem fs;
    fs.createDirectory("/data");
    fs.createFile("/data/file1.txt", "1", 1);
    fs.createFile("/data/file2.txt", "2", 1);
    DirectoryNavigator nav(&fs);

    auto files = nav.ls("/data");
    TEST_ASSERT_EQUAL(2, files.size());
}

// Test mkdir absolute path
void test_navigator_mkdir_absolute(void)
{
    MockFileSystem fs;
    DirectoryNavigator nav(&fs);

    bool result = nav.mkdir("/newdir");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(fs.exists("/newdir"));
}

// Test mkdir relative path
void test_navigator_mkdir_relative(void)
{
    MockFileSystem fs;
    fs.createDirectory("/home");
    DirectoryNavigator nav(&fs, "/home");

    bool result = nav.mkdir("subdir");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(fs.exists("/home/subdir"));
}

// Test rmdir
void test_navigator_rmdir(void)
{
    MockFileSystem fs;
    fs.createDirectory("/tempdir");
    DirectoryNavigator nav(&fs);

    bool result = nav.rmdir("/tempdir");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(fs.exists("/tempdir"));
}

// Test remove file
void test_navigator_remove(void)
{
    MockFileSystem fs;
    fs.createFile("/file.txt", "data", 4);
    DirectoryNavigator nav(&fs);

    bool result = nav.remove("/file.txt");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(fs.exists("/file.txt"));
}

// Test isDirectory
void test_navigator_is_directory(void)
{
    MockFileSystem fs;
    fs.createDirectory("/dir");
    fs.createFile("/file.txt", "data", 4);
    DirectoryNavigator nav(&fs);

    TEST_ASSERT_TRUE(nav.isDirectory("/dir"));
    TEST_ASSERT_FALSE(nav.isDirectory("/file.txt"));
    TEST_ASSERT_FALSE(nav.isDirectory("/nonexistent"));
}

// Test isEmpty
void test_navigator_is_empty(void)
{
    MockFileSystem fs;
    fs.createDirectory("/empty");
    fs.createDirectory("/nonempty");
    fs.createFile("/nonempty/file.txt", "data", 4);
    DirectoryNavigator nav(&fs);

    TEST_ASSERT_TRUE(nav.isEmpty("/empty"));
    TEST_ASSERT_FALSE(nav.isEmpty("/nonempty"));
}

// Test pwd with path (resolve path)
void test_navigator_pwd_with_path(void)
{
    MockFileSystem fs;
    fs.createDirectory("/home");
    DirectoryNavigator nav(&fs, "/home");

    ETString resolved = nav.pwd("subdir");
    TEST_ASSERT_EQUAL_STRING("/home/subdir", resolved.c_str());
}

// Test getFileSystem accessor
void test_navigator_get_filesystem(void)
{
    MockFileSystem fs;
    DirectoryNavigator nav(&fs);

    IFileSystem *retrieved = nav.getFileSystem();
    TEST_ASSERT_EQUAL_PTR(&fs, retrieved);
}

// Test complex path resolution with .. and .
void test_navigator_complex_path_resolution(void)
{
    MockFileSystem fs;
    fs.createDirectory("/a");
    fs.createDirectory("/a/b");
    fs.createDirectory("/a/b/c");
    DirectoryNavigator nav(&fs, "/a/b/c");

    ETString resolved = nav.pwd("../../x");
    TEST_ASSERT_EQUAL_STRING("/a/x", resolved.c_str());
}

// Test cd with . (current directory)
void test_navigator_cd_current(void)
{
    MockFileSystem fs;
    fs.createDirectory("/home");
    DirectoryNavigator nav(&fs, "/home");

    bool result = nav.cd(".");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("/home", nav.pwd().c_str());
}

// Test path normalization (multiple slashes)
void test_navigator_path_normalization(void)
{
    MockFileSystem fs;
    fs.createDirectory("/home");
    DirectoryNavigator nav(&fs);

    ETString resolved = nav.pwd("//home//");
    // Should normalize to /home
    TEST_ASSERT_TRUE(resolved.find("/home") != ETString::npos);
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_navigator_construction);
    RUN_TEST(test_navigator_custom_root);
    RUN_TEST(test_navigator_cd_absolute);
    RUN_TEST(test_navigator_cd_relative);
    RUN_TEST(test_navigator_cd_parent);
    RUN_TEST(test_navigator_cd_nonexistent);
    RUN_TEST(test_navigator_cd_to_file);
    RUN_TEST(test_navigator_exists_absolute);
    RUN_TEST(test_navigator_exists_relative);
    RUN_TEST(test_navigator_ls_current);
    RUN_TEST(test_navigator_ls_path);
    RUN_TEST(test_navigator_mkdir_absolute);
    RUN_TEST(test_navigator_mkdir_relative);
    RUN_TEST(test_navigator_rmdir);
    RUN_TEST(test_navigator_remove);
    RUN_TEST(test_navigator_is_directory);
    RUN_TEST(test_navigator_is_empty);
    RUN_TEST(test_navigator_pwd_with_path);
    RUN_TEST(test_navigator_get_filesystem);
    RUN_TEST(test_navigator_complex_path_resolution);
    RUN_TEST(test_navigator_cd_current);
    RUN_TEST(test_navigator_path_normalization);
    UNITY_END();
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
    process_tests();
    return 0;
}
#endif

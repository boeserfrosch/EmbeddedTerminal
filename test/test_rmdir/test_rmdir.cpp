// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "commands/rmdir.h"
#include <string>
#include "../Mocks/MockFileSystem.h"
#include "DirectoryNavigator.h"

void setUp(void) {}
void tearDown(void) {}
MockFileSystem FS;
DirectoryNavigator dir(&FS);

class TestRmdir : public cmd::rmdir
{
public:
    TestRmdir() : cmd::rmdir(dir)
    {
        // Setup mock directories
        FS.createDirectory("dir1");
        FS.createFile("file.txt", "hello");
        FS.createDirectory("foo/bar/baz");
    }

    bool exists(ETString path)
    {
        return FS.exists(path);
    }
};

void test_rmdir_valid_directory(void)
{
    TestRmdir rmdir;
    ETString keyword = "rmdir";
    ETString arg = "dir1";
    ETString result = rmdir.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("removed") != ETString::npos);
}

void test_rmdir_nonexistent_directory(void)
{
    TestRmdir rmdir;
    ETString keyword = "rmdir";
    ETString arg = "no_dir";
    ETString result = rmdir.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("did not exist") != ETString::npos);
}

void test_rmdir_file_instead_of_directory(void)
{
    TestRmdir rmdir;
    ETString keyword = "rmdir";
    ETString arg = "file.txt";
    ETString result = rmdir.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("not a directory") != ETString::npos);
}

void test_rmdir_usage(void)
{
    TestRmdir rmdir;
    ETString keyword = "rmdir";
    ETString result = rmdir.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Remove the specfied folder") != ETString::npos);
}

void test_rmdir_edge_cases(void)
{
    TestRmdir rmdir;
    ETString keyword = "rmdir";
    ETString arg = "   ";
    ETString result = rmdir.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("Can not remove folder with no name\n") != ETString::npos);
}

void test_rmdir_subdirectory(void)
{
    TestRmdir rmdir;
    ETString keyword = "rmdir";
    ETString arg = "/foo/bar/baz";
    ETString result = rmdir.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("removed") != ETString::npos);
    TEST_ASSERT_TRUE(rmdir.exists("/foo/bar"));
    TEST_ASSERT_FALSE(rmdir.exists("/foo/bar/baz"));
}

void test_rmdir_notemptydirectory(void)
{
    TestRmdir rmdir;
    ETString keyword = "rmdir";
    ETString arg = "/foo/bar";
    ETString result = rmdir.trigger(keyword, arg);
    TEST_ASSERT_TRUE(result.find("is not empty") != ETString::npos);
    TEST_ASSERT_TRUE(rmdir.exists("/foo/bar/baz"));
    TEST_ASSERT_TRUE(rmdir.exists("/foo/bar"));
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_rmdir_valid_directory);
    RUN_TEST(test_rmdir_nonexistent_directory);
    RUN_TEST(test_rmdir_file_instead_of_directory);
    RUN_TEST(test_rmdir_usage);
    RUN_TEST(test_rmdir_edge_cases);
    RUN_TEST(test_rmdir_subdirectory);
    RUN_TEST(test_rmdir_notemptydirectory);
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

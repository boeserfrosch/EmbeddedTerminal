// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <unity.h>

#include "../src/hal/SDMMCFileSystem.h"
#include "SD_MMC.h"

using namespace EmbeddedTerminal;

static SDMMCFileSystem *fs;

void setUp(void)
{
    if (!SD_MMC.begin())
    {
        TEST_FAIL_MESSAGE("SD init failed, cannot run FileSystem tests!");
    }
    fs = new SDMMCFileSystem();

    // Aufräumen vor jedem Test
    if (fs->exists("/test.txt"))
        fs->remove("/test.txt");
    if (fs->exists("/dir"))
        fs->rmdir("/dir");
}

void tearDown(void)
{
    delete fs;
}

// ---- Tests mit gültigem Mount ----

void test_open_and_write_read_file(void)
{
    ETFile file = fs->open("/test.txt", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(file.isOpen());
    TEST_ASSERT_TRUE(file.writeAll("Hello World!"));
    file.close();

    ETFile file2 = fs->open("/test.txt", FILE_MODE_READ);
    TEST_ASSERT_TRUE(file2.isOpen());
    ETString content = file2.readAll();
    TEST_ASSERT_EQUAL_STRING("Hello World!", content.c_str());
    file2.close();
}

void test_exists_and_remove(void)
{
    ETFile file = fs->open("/test.txt", FILE_MODE_WRITE, true);
    file.writeAll("abc");
    file.close();

    TEST_ASSERT_TRUE(fs->exists("/test.txt"));
    TEST_ASSERT_TRUE(fs->remove("/test.txt"));
    TEST_ASSERT_FALSE(fs->exists("/test.txt"));
}

void test_isEmpty(void)
{
    ETFile file = fs->open("/test.txt", FILE_MODE_WRITE, true);
    file.writeAll("");
    file.close();

    TEST_ASSERT_TRUE(fs->isEmpty("/test.txt"));

    ETFile file2 = fs->open("/test.txt", FILE_MODE_WRITE);
    file2.writeAll("data");
    file2.close();

    TEST_ASSERT_FALSE(fs->isEmpty("/test.txt"));
}

void test_mkdir_and_rmdir(void)
{
    TEST_ASSERT_FALSE(fs->exists("/dir"));
    TEST_ASSERT_TRUE(fs->mkdir("/dir"));
    TEST_ASSERT_TRUE(fs->exists("/dir"));
    TEST_ASSERT_TRUE(fs->isDirectory("/dir"));
    TEST_ASSERT_TRUE(fs->rmdir("/dir"));
    TEST_ASSERT_FALSE(fs->exists("/dir"));
}

// ---- Fehlerfall Tests ----

void test_open_with_null_mount_returns_invalid(void)
{
    SDMMCFileSystem nullFs; // _mount == nullptr
    ETFile file = nullFs.open("/invalid.txt", FILE_MODE_READ);
    TEST_ASSERT_FALSE(file.isOpen());
}

void test_exists_with_null_mount_returns_false(void)
{
    SDMMCFileSystem nullFs;
    TEST_ASSERT_FALSE(nullFs.exists("/anything.txt"));
}

void test_remove_with_null_mount_returns_false(void)
{
    SDMMCFileSystem nullFs;
    TEST_ASSERT_FALSE(nullFs.remove("/anything.txt"));
}

void test_mkdir_with_null_mount_returns_false(void)
{
    SDMMCFileSystem nullFs;
    TEST_ASSERT_FALSE(nullFs.mkdir("/someDir"));
}

void test_rmdir_with_null_mount_returns_false(void)
{
    SDMMCFileSystem nullFs;
    TEST_ASSERT_FALSE(nullFs.rmdir("/someDir"));
}

void test_isDirectory_with_null_mount_returns_false(void)
{
    SDMMCFileSystem nullFs;
    TEST_ASSERT_FALSE(nullFs.isDirectory("/someDir"));
}

void test_isEmpty_with_null_mount_returns_true(void)
{
    SDMMCFileSystem nullFs;
    TEST_ASSERT_TRUE(nullFs.isEmpty("/someFile")); // Da Datei nicht existiert
}

int process_tests_filesystem()
{
    UNITY_BEGIN();
    // Normal cases
    RUN_TEST(test_open_and_write_read_file);
    RUN_TEST(test_exists_and_remove);
    RUN_TEST(test_isEmpty);
    RUN_TEST(test_mkdir_and_rmdir);

    // Error cases
    RUN_TEST(test_open_with_null_mount_returns_invalid);
    RUN_TEST(test_exists_with_null_mount_returns_false);
    RUN_TEST(test_remove_with_null_mount_returns_false);
    RUN_TEST(test_mkdir_with_null_mount_returns_false);
    RUN_TEST(test_rmdir_with_null_mount_returns_false);
    RUN_TEST(test_isDirectory_with_null_mount_returns_false);
    RUN_TEST(test_isEmpty_with_null_mount_returns_true);

    return UNITY_END();
}

// ---- Arduino entry points ----
#if defined(ARDUINO)
void setup()
{
    delay(2000); // Give serial monitor time to connect
    process_tests_filesystem();
}
void loop() {}
#else
int main()
{
    return process_tests_filesystem();
}
#endif

// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#include "SD_MMC.h"
#endif
#include <unity.h>

#include "../src/hal/SDMMCFileSystem.h"

using namespace EmbeddedTerminal;

#if defined(ARDUINO)

static SDMMCFileSystem *fileSystem;

void setUp(void)
{
    if (!SD_MMC.begin())
    {
        TEST_FAIL_MESSAGE("SD init failed, cannot run FileSystem tests!");
    }
    fileSystem = new SDMMCFileSystem();

    // Aufräumen vor jedem Test
    if (fileSystem->exists("/test.txt"))
        fileSystem->remove("/test.txt");
    if (fileSystem->exists("/dir"))
        fileSystem->rmdir("/dir");
}

void tearDown(void)
{
    delete fileSystem;
}

// ---- Tests mit gültigem Mount ----

void test_open_and_write_read_file(void)
{
    ETFile file = fileSystem->open("/test.txt", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(file.isOpen());
    TEST_ASSERT_TRUE(file.writeAll("Hello World!"));
    file.close();

    ETFile file2 = fileSystem->open("/test.txt", FILE_MODE_READ);
    TEST_ASSERT_TRUE(file2.isOpen());
    ETString content = file2.readAll();
    TEST_ASSERT_EQUAL_STRING("Hello World!", content.c_str());
    file2.close();
}

void test_exists_and_remove(void)
{
    ETFile file = fileSystem->open("/test.txt", FILE_MODE_WRITE, true);
    file.writeAll("abc");
    file.close();

    TEST_ASSERT_TRUE(fileSystem->exists("/test.txt"));
    TEST_ASSERT_TRUE(fileSystem->remove("/test.txt"));
    TEST_ASSERT_FALSE(fileSystem->exists("/test.txt"));
}

void test_isEmpty(void)
{
    ETFile file = fileSystem->open("/test.txt", FILE_MODE_WRITE, true);
    file.writeAll("");
    file.close();

    TEST_ASSERT_TRUE(fileSystem->isEmpty("/test.txt"));

    ETFile file2 = fileSystem->open("/test.txt", FILE_MODE_WRITE);
    file2.writeAll("data");
    file2.close();

    TEST_ASSERT_FALSE(fileSystem->isEmpty("/test.txt"));
}

void test_mkdir_and_rmdir(void)
{
    TEST_ASSERT_FALSE(fileSystem->exists("/dir"));
    TEST_ASSERT_TRUE(fileSystem->mkdir("/dir"));
    TEST_ASSERT_TRUE(fileSystem->exists("/dir"));
    TEST_ASSERT_TRUE(fileSystem->isDirectory("/dir"));
    TEST_ASSERT_TRUE(fileSystem->rmdir("/dir"));
    TEST_ASSERT_FALSE(fileSystem->exists("/dir"));
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

#else // !ARDUINO (ESP-IDF)

// SD_MMC is not available in ESP-IDF, provide stub implementations
void setUp(void) {}
void tearDown(void) {}

void test_sdmmc_not_available_in_espidf(void)
{
    TEST_IGNORE_MESSAGE("SD_MMC tests only available on Arduino framework");
}

#endif // ARDUINO

int process_tests_filesystem()
{
    UNITY_BEGIN();
#if defined(ARDUINO)
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
#else
    // ESP-IDF: SD_MMC not available
    RUN_TEST(test_sdmmc_not_available_in_espidf);
#endif

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

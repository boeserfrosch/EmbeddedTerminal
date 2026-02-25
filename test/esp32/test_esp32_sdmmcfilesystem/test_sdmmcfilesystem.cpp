// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#include "SD_MMC.h"
#endif
#include <unity.h>

#include "../src/hal/SDMMCFileSystem.h"
#include "../test/utils/SD.h"

#if defined(ARDUINO) || defined(ESP_PLATFORM) || defined(ESP_32)

using namespace EmbeddedTerminal;

static SDMMCFileSystem *fileSystem;

void setUp(void)
{
    if (!setup_sdmmc())
    {
        TEST_FAIL_MESSAGE("SD init failed, cannot run FileSystem tests!");
    }
#if defined(ARDUINO)
    fileSystem = new SDMMCFileSystem(card);
#elif defined(ESP_PLATFORM) || defined(ESP_32)
    fileSystem = new SDMMCFileSystem(card, ETString(SDMMC_MOUNT_POINT));
#endif
    // Aufräumen vor jedem Test
    if (fileSystem->exists("/test.txt"))
        fileSystem->remove("/test.txt");
    if (fileSystem->exists("/dir"))
        fileSystem->rmdir("/dir");
    if (fileSystem->exists("/someFile"))
        fileSystem->remove("/someFile");
    if (fileSystem->exists("/someDir/someFile.txt"))
        fileSystem->remove("/someDir/someFile.txt");
    if (fileSystem->exists("/someDir"))
        fileSystem->rmdir("/someDir");
}

void tearDown(void)
{
    delete fileSystem;
    teardown_sdmmc();
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

void test_open_returns_invalid(void)
{
    ETFile file = fileSystem->open("/invalid.txt", FILE_MODE_READ);
    TEST_ASSERT_FALSE(file.isOpen());
    file.close(); // Should not crash even if file is invalid
    TEST_ASSERT_FALSE(file.isOpen());
}

void test_exists_returns_false(void)
{
    TEST_ASSERT_FALSE(fileSystem->exists("/anything.txt"));
}

void test_remove_returns_false(void)
{
    TEST_ASSERT_FALSE(fileSystem->remove("/anything.txt"));
}

void test_mkdir_returns_true(void)
{
    TEST_ASSERT_FALSE(fileSystem->exists("/someOtherDir"));
    TEST_ASSERT_TRUE(fileSystem->mkdir("/someOtherDir"));
    TEST_ASSERT_TRUE(fileSystem->exists("/someOtherDir"));
}

void test_rmdir_returns_false(void)
{
    TEST_ASSERT_FALSE(fileSystem->rmdir("/someDir"));
}

void test_isDirectory_returns_false(void)
{
    TEST_ASSERT_FALSE(fileSystem->isDirectory("/someDir"));
}

void test_isEmpty_returns_true(void)
{

    TEST_ASSERT_FALSE(fileSystem->isEmpty("/someFile")); // Da Datei nicht existiert
    fileSystem->mkdir("/someDir");
    TEST_ASSERT_TRUE(fileSystem->isEmpty("/someDir")); // Da Verzeichnis leer ist

    fileSystem->open("/someDir/someFile.txt", FILE_MODE_WRITE, true).writeAll("data");
    TEST_ASSERT_FALSE(fileSystem->isEmpty("/someDir")); // Da Verzeichnis jetzt nicht mehr leer ist
    fileSystem->remove("/someDir/someFile.txt");
    TEST_ASSERT_TRUE(fileSystem->isEmpty("/someDir")); // Da Verzeichnis jetzt wieder leer ist
    fileSystem->rmdir("/someDir");
    TEST_ASSERT_FALSE(fileSystem->isEmpty("/someDir")); // Da Verzeichnis jetzt nicht mehr existiert
}

#endif // ARDUINO

void test_sdmmc_not_available(void)
{
    TEST_ASSERT_MESSAGE(true, "SDMMC not available on this platform, skipping tests!");
}

int process_tests_filesystem()
{
    UNITY_BEGIN();
#if defined(ARDUINO) || defined(ESP_PLATFORM) || defined(ESP_32)
    // Normal cases
    RUN_TEST(test_open_and_write_read_file);
    RUN_TEST(test_exists_and_remove);
    RUN_TEST(test_isEmpty);
    RUN_TEST(test_mkdir_and_rmdir);

    // Error cases
    RUN_TEST(test_open_returns_invalid);
    RUN_TEST(test_exists_returns_false);
    RUN_TEST(test_remove_returns_false);
    RUN_TEST(test_mkdir_returns_true);
    RUN_TEST(test_rmdir_returns_false);
    RUN_TEST(test_isDirectory_returns_false);
    RUN_TEST(test_isEmpty_returns_true);
#else
    RUN_TEST(test_sdmmc_not_available);
#endif

    return UNITY_END();
}

// ---- Arduino entry points ----
#if defined(ARDUINO)
void setup()
{
    delay(3000); // Give serial monitor time to connect
    process_tests_filesystem();
}
void loop()
{
}
#elif (defined(ESP_PLATFORM) || defined(ESP_32))
extern "C" void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(4000));
    process_tests_filesystem();
}
#else
int main()
{
    return process_tests_filesystem();
}
#endif

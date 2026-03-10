#include <unity.h>
#include "interfaces/IStorage.h"
#include "ETTypes.h"
#include "../../Mocks/native/MockStorageMedia.h"
#include "../../../src/StorageSystem.h"

using namespace EmbeddedTerminal;

IStorageSystem *storageSystemUnderTest;

void test_media_list()
{
    MockStorageMedia m1("SD", true, 1000, 500, 1000, 500, nullptr);
    MockStorageMedia m2("Flash", true, 2000, 1000, 2000, 1000, nullptr);
    storageSystemUnderTest->mountMedia(&m1, "/SD");
    storageSystemUnderTest->mountMedia(&m2, "/Flash");

    auto mediaList = storageSystemUnderTest->media();
    TEST_ASSERT_EQUAL(2, mediaList.size());
    // Names may be in any order depending on implementation
    bool foundSD = false, foundFlash = false;
    for (auto *m : mediaList)
    {
        if (strcmp(m->name(), "SD") == 0)
            foundSD = true;
        if (strcmp(m->name(), "Flash") == 0)
            foundFlash = true;
    }
    TEST_ASSERT_TRUE(foundSD);
    TEST_ASSERT_TRUE(foundFlash);
}

void test_getMedia()
{
    MockStorageMedia m1("SD", true, 1000, 500, 1000, 500, nullptr);
    MockStorageMedia m2("Flash", true, 2000, 1000, 2000, 1000, nullptr);
    // StorageSystem expects mount point == name, MockStorageSystem expects "/mnt/SD"
    storageSystemUnderTest->mountMedia(&m1, "/SD");
    storageSystemUnderTest->mountMedia(&m2, "/Flash");
    IStorageMedia *sd = storageSystemUnderTest->getMedia("SD");
    IStorageMedia *flash = storageSystemUnderTest->getMedia("Flash");
    // Accept either pointer or nullptr if not found (depends on implementation)
    TEST_ASSERT_TRUE_MESSAGE(sd == &m1, "getMedia(\"SD\") should return pointer to m1");
    TEST_ASSERT_TRUE_MESSAGE(flash == &m2, "getMedia(\"Flash\") should return pointer to m2");
    TEST_ASSERT_NULL(storageSystemUnderTest->getMedia("USB"));
}

void test_getMediaFromPath()
{
    MockStorageMedia m1("SD", true, 1000, 500, 1000, 500, nullptr);
    storageSystemUnderTest->mountMedia(&m1, "/SD");
    // StorageSystem expects "/SD/file.txt", MockStorageSystem may expect "/mnt/SD/file.txt"
    IStorageMedia *media1 = storageSystemUnderTest->getMediaFromPath("/SD/file.txt");
    TEST_ASSERT_FALSE_MESSAGE(media1 == nullptr, "getMediaFromPath(\"/SD/file.txt\") should not return nullptr");
    TEST_ASSERT(media1 == &m1);
    TEST_ASSERT_NULL(storageSystemUnderTest->getMediaFromPath("/other/file.txt"));
}

void test_mountMedia()
{
    MockStorageMedia m1("SD", true, 1000, 500, 1000, 500, nullptr);
    storageSystemUnderTest->mountMedia(&m1, "/SD");
    // Try mounting a new media
    MockStorageMedia m2("Flash", true, 2000, 1000, 2000, 1000, nullptr);
    bool mountRes = storageSystemUnderTest->mountMedia(&m2, "/Flash");
    TEST_ASSERT_TRUE(mountRes);
    // Should now be accessible via media()
    auto mediaList = storageSystemUnderTest->media();
    bool foundFlash = false;
    for (auto *m : mediaList)
    {
        if (strcmp(m->name(), "Flash") == 0)
            foundFlash = true;
    }
    TEST_ASSERT_TRUE(foundFlash);
}

void test_copy_move_remove()
{
    MockStorageMedia m1("SD", true, 1000, 500, 1000, 500, new MockFileSystem());
    storageSystemUnderTest->mountMedia(&m1, "/SD");

    bool copyRes = storageSystemUnderTest->copyFile("/SD/src.txt", "/SD/dst.txt");
    storageSystemUnderTest->open("/SD/src.txt", "w", true);
    bool copyRes2 = storageSystemUnderTest->copyFile("/SD/src.txt", "/SD/dst.txt");
    bool moveRes = storageSystemUnderTest->moveFile("/SD/src.txt", "/SD/dst.txt");
    bool moveRes2 = storageSystemUnderTest->moveFile("/SD/src.txt", "/SD/dst2.txt");
    bool removeRes = storageSystemUnderTest->removeFile("/SD/dst.txt");

    TEST_ASSERT_FALSE(copyRes);  // False because src.txt does not exist
    TEST_ASSERT_TRUE(copyRes2);  // True because src.txt now exists
    TEST_ASSERT_TRUE(moveRes);   // Moving should succeed even if dst.txt already exists after copy
    TEST_ASSERT_FALSE(moveRes2); // False because src.txt was moved and no longer exists
    TEST_ASSERT_TRUE(removeRes);
}

// Additional tests for IStorageSystem

void test_media_capacity()
{
    MockStorageMedia m1("SD", true, 1000, 500, 1000, 500, new MockFileSystem());
    MockStorageMedia m2("Flash", true, 2000, 1000, 2000, 1000, new MockFileSystem());
    storageSystemUnderTest->mountMedia(&m1, "/SD");
    storageSystemUnderTest->mountMedia(&m2, "/Flash");
    auto mediaList = storageSystemUnderTest->media();
    for (auto *m : mediaList)
    {
        if (strcmp(m->name(), "SD") == 0)
        {
            TEST_ASSERT_EQUAL_UINT32(1000, m->capacity());
            TEST_ASSERT_EQUAL_UINT32(500, m->freeBytes());
        }
        if (strcmp(m->name(), "Flash") == 0)
        {
            TEST_ASSERT_EQUAL_UINT32(2000, m->capacity());
            TEST_ASSERT_EQUAL_UINT32(1000, m->freeBytes());
        }
    }
}

void test_unmount_media()
{
    MockStorageMedia m1("SD", true, 1000, 500, 1000, 500, new MockFileSystem());
    storageSystemUnderTest->mountMedia(&m1, "/SD");
    // Unmount media
    bool unmountRes = storageSystemUnderTest->unmountMedia("SD");
    TEST_ASSERT_TRUE(unmountRes); // Accept either for mock/real
    // Should not be accessible anymore
    TEST_ASSERT_NULL(storageSystemUnderTest->getMedia("SD"));
}

void setUp(void)
{
    // This is run before EACH TEST
    storageSystemUnderTest = new StorageSystem();
}

void tearDown(void)
{
    // This is run after EACH TEST
    delete storageSystemUnderTest;
}

int run_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_media_list);
    RUN_TEST(test_getMedia);
    RUN_TEST(test_getMediaFromPath);
    RUN_TEST(test_mountMedia);
    RUN_TEST(test_copy_move_remove);
    RUN_TEST(test_media_capacity);
    RUN_TEST(test_unmount_media);
    return UNITY_END();
}
#if defined(ARDUINO)

void setup()
{
    delay(4000); // Wait for serial to be ready
    run_tests();
}
void loop() {}

#elif defined(ESP_PLATFORM) || defined(ESP32)

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
extern "C" void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(4000));
    run_tests();
}

#else

int main()
{
    return run_tests();
}

#endif

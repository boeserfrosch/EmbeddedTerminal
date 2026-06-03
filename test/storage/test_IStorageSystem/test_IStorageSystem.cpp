#include <unity.h>
#include "interfaces/IStorage.h"
#include "ETTypes.h"
#include "../../Mocks/native/MockStorageMedia.h"
#include "../../../src/StorageSystem.h"
#include "../../../src/DirectoryNavigator.h"

using namespace EmbeddedTerminal;

IStorageSystem *storageSystemUnderTest;

void test_media_list()
{
    auto m1 = std::make_shared<MockStorageMedia>("SD", true, 1000, 500, 1000, 500, nullptr);
    auto m2 = std::make_shared<MockStorageMedia>("Flash", true, 2000, 1000, 2000, 1000, nullptr);
    storageSystemUnderTest->mountMedia(m1, "/SD");
    storageSystemUnderTest->mountMedia(m2, "/Flash");

    auto mediaList = storageSystemUnderTest->media();
    TEST_ASSERT_EQUAL(2, mediaList.size());
    // Names may be in any order depending on implementation
    bool foundSD = false, foundFlash = false;
    for (auto m : mediaList)
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
    auto m1 = std::make_shared<MockStorageMedia>("SD", true, 1000, 500, 1000, 500, nullptr);
    auto m2 = std::make_shared<MockStorageMedia>("Flash", true, 2000, 1000, 2000, 1000, nullptr);
    // StorageSystem expects mount point == name, MockStorageSystem expects "/mnt/SD"
    storageSystemUnderTest->mountMedia(m1, "/SD");
    storageSystemUnderTest->mountMedia(m2, "/Flash");
    auto sd = storageSystemUnderTest->getMedia("SD");
    auto flash = storageSystemUnderTest->getMedia("Flash");
    // Verify by name rather than pointer equality
    TEST_ASSERT_NOT_NULL(sd.get());
    TEST_ASSERT_NOT_NULL(flash.get());
    TEST_ASSERT_EQUAL_STRING("SD", sd->name());
    TEST_ASSERT_EQUAL_STRING("Flash", flash->name());
    TEST_ASSERT_TRUE(!storageSystemUnderTest->getMedia("USB"));
}

void test_getMediaFromPath()
{
    auto m1 = std::make_shared<MockStorageMedia>("SD", true, 1000, 500, 1000, 500, nullptr);
    storageSystemUnderTest->mountMedia(m1, "/SD");
    // StorageSystem expects "/SD/file.txt", MockStorageSystem may expect "/mnt/SD/file.txt"
    auto media1 = storageSystemUnderTest->getMediaFromPath("/SD/file.txt");
    TEST_ASSERT_FALSE_MESSAGE(!media1, "getMediaFromPath(\"/SD/file.txt\") should not return nullptr");
    TEST_ASSERT_EQUAL_STRING("SD", media1->name());
    TEST_ASSERT_TRUE(!storageSystemUnderTest->getMediaFromPath("/other/file.txt"));
}

void test_open_on_mounted_path_strips_mount_prefix()
{
    auto m1 = std::make_shared<MockStorageMedia>("SD", true, 1000, 500, 1000, 500, new MockFileSystem());
    storageSystemUnderTest->mountMedia(m1, "/SD");

    ETFile file = storageSystemUnderTest->open("/SD/file.txt", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE_MESSAGE(file.isOpen(), "Expected mounted path to open successfully");
    TEST_ASSERT_TRUE_MESSAGE(file.writeAll("hello"), "Expected write through mounted path to succeed");
    file.close();

    ETString content = storageSystemUnderTest->open("/SD/file.txt", FILE_MODE_READ).readAll();
    TEST_ASSERT_EQUAL_STRING("hello", content.c_str());
}

void test_mountMedia()
{
    auto m1 = std::make_shared<MockStorageMedia>("SD", true, 1000, 500, 1000, 500, nullptr);
    storageSystemUnderTest->mountMedia(m1, "/SD");
    // Try mounting a new media
    auto m2 = std::make_shared<MockStorageMedia>("Flash", true, 2000, 1000, 2000, 1000, nullptr);
    bool mountRes = storageSystemUnderTest->mountMedia(m2, "/Flash");
    TEST_ASSERT_TRUE(mountRes);
    // Should now be accessible via media()
    auto mediaList = storageSystemUnderTest->media();
    bool foundFlash = false;
    for (auto m : mediaList)
    {
        if (strcmp(m->name(), "Flash") == 0)
            foundFlash = true;
    }
    TEST_ASSERT_TRUE(foundFlash);
}

void test_mountMedia_rejects_null_media()
{
    bool mountRes = storageSystemUnderTest->mountMedia(nullptr, "/SD");
    TEST_ASSERT_FALSE(mountRes);
    TEST_ASSERT_EQUAL(0, storageSystemUnderTest->media().size());
}

void test_copy_move_remove()
{
    auto m1 = std::make_shared<MockStorageMedia>("SD", true, 1000, 500, 1000, 500, new MockFileSystem());
    storageSystemUnderTest->mountMedia(m1, "/SD");

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
    auto m1 = std::make_shared<MockStorageMedia>("SD", true, 1000, 500, 1000, 500, new MockFileSystem());
    auto m2 = std::make_shared<MockStorageMedia>("Flash", true, 2000, 1000, 2000, 1000, new MockFileSystem());
    storageSystemUnderTest->mountMedia(m1, "/SD");
    storageSystemUnderTest->mountMedia(m2, "/Flash");
    auto mediaList = storageSystemUnderTest->media();
    for (auto m : mediaList)
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
    auto m1 = std::make_shared<MockStorageMedia>("SD", true, 1000, 500, 1000, 500, new MockFileSystem());
    storageSystemUnderTest->mountMedia(m1, "/SD");
    // Unmount media
    bool unmountRes = storageSystemUnderTest->unmountMedia("SD");
    TEST_ASSERT_TRUE(unmountRes); // Accept either for mock/real
    // Should not be accessible anymore
    TEST_ASSERT_TRUE(!storageSystemUnderTest->getMedia("SD"));
}

void test_exists_with_variation_in_path_formats()
{
    auto m1 = std::make_shared<MockStorageMedia>("", true, 1000, 500, 1000, 500, new MockFileSystem());
    auto m2 = std::make_shared<MockStorageMedia>("SD", true, 1000, 500, 1000, 500, new MockFileSystem());
    storageSystemUnderTest->mountMedia(m1, "/");
    storageSystemUnderTest->mountMedia(m2, "/SD");
    storageSystemUnderTest->open("/file.txt", "w", true).writeAll("test");
    storageSystemUnderTest->open("/SD/file.txt", "w", true).writeAll("test");

    // Test exists with different path formats
    TEST_ASSERT_TRUE(storageSystemUnderTest->exists("/file.txt"));
    TEST_ASSERT_TRUE(storageSystemUnderTest->exists("file.txt"));
    TEST_ASSERT_TRUE(storageSystemUnderTest->exists("/SD/file.txt"));
    TEST_ASSERT_TRUE(storageSystemUnderTest->exists("SD/file.txt"));
    TEST_ASSERT_TRUE(storageSystemUnderTest->exists("/SD//file.txt"));
    TEST_ASSERT_TRUE(storageSystemUnderTest->exists("/SD/./file.txt"));
    TEST_ASSERT_TRUE(storageSystemUnderTest->exists("/SD/../SD/file.txt"));
    TEST_ASSERT_FALSE(storageSystemUnderTest->exists("/SD/nonexistent.txt"));
}

// ===== StorageSystem + DirectoryNavigator Lifecycle Tests =====

void test_create_write_read_delete_lifecycle(void)
{
    auto m1 = std::make_shared<MockStorageMedia>("", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storageSystemUnderTest->mountMedia(m1, "/");

    DirectoryNavigator nav(storageSystemUnderTest);

    // Create and write
    auto file = storageSystemUnderTest->open("/testfile.txt", "w", true);
    file.writeAll("Hello World");
    file.close();

    // Verify exists
    TEST_ASSERT_TRUE(nav.exists("/testfile.txt"));

    // Read and verify
    ETString content = storageSystemUnderTest->open("/testfile.txt", "r").readAll();
    TEST_ASSERT_EQUAL_STRING("Hello World", content.c_str());

    // Delete
    TEST_ASSERT_TRUE(nav.remove("/testfile.txt"));
    TEST_ASSERT_FALSE(nav.exists("/testfile.txt"));
}

void test_nested_directory_create_and_navigate(void)
{
    auto m1 = std::make_shared<MockStorageMedia>("", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storageSystemUnderTest->mountMedia(m1, "/");

    DirectoryNavigator nav(storageSystemUnderTest);

    // Create nested structure
    TEST_ASSERT_TRUE(nav.mkdir("/project"));
    TEST_ASSERT_TRUE(nav.mkdir("/project/src"));
    TEST_ASSERT_TRUE(nav.mkdir("/project/src/lib"));

    // Navigate and verify
    TEST_ASSERT_TRUE(nav.cd("/project"));
    TEST_ASSERT_EQUAL_STRING("/project", nav.pwd().c_str());

    TEST_ASSERT_TRUE(nav.cd("src"));
    TEST_ASSERT_EQUAL_STRING("/project/src", nav.pwd().c_str());

    TEST_ASSERT_TRUE(nav.cd("lib"));
    TEST_ASSERT_EQUAL_STRING("/project/src/lib", nav.pwd().c_str());
}

void test_write_across_nested_directories(void)
{
    auto m1 = std::make_shared<MockStorageMedia>("", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storageSystemUnderTest->mountMedia(m1, "/");

    DirectoryNavigator nav(storageSystemUnderTest);

    // Create structure
    nav.mkdir("/docs");
    nav.mkdir("/docs/sections");

    // Write files
    storageSystemUnderTest->open("/docs/readme.txt", "w", true).writeAll("Main readme");
    storageSystemUnderTest->open("/docs/sections/chapter1.txt", "w", true).writeAll("Chapter 1");

    // Verify files exist
    TEST_ASSERT_TRUE(nav.exists("/docs/readme.txt"));
    TEST_ASSERT_TRUE(nav.exists("/docs/sections/chapter1.txt"));
}

void test_multiple_files_in_directory_lifecycle(void)
{
    auto m1 = std::make_shared<MockStorageMedia>("", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storageSystemUnderTest->mountMedia(m1, "/");

    DirectoryNavigator nav(storageSystemUnderTest);

    // Create directory
    TEST_ASSERT_TRUE(nav.mkdir("/data"));

    // Create multiple files
    storageSystemUnderTest->open("/data/file1.txt", "w", true).writeAll("content1");
    storageSystemUnderTest->open("/data/file2.txt", "w", true).writeAll("content2");
    storageSystemUnderTest->open("/data/file3.txt", "w", true).writeAll("content3");

    // List and verify count
    auto files = nav.ls("/data");
    TEST_ASSERT_EQUAL(3, files.size());

    // Verify contents
    TEST_ASSERT_EQUAL_STRING("content1", storageSystemUnderTest->open("/data/file1.txt", "r").readAll().c_str());
    TEST_ASSERT_EQUAL_STRING("content2", storageSystemUnderTest->open("/data/file2.txt", "r").readAll().c_str());
    TEST_ASSERT_EQUAL_STRING("content3", storageSystemUnderTest->open("/data/file3.txt", "r").readAll().c_str());

    // Delete all
    TEST_ASSERT_TRUE(nav.remove("/data/file1.txt"));
    TEST_ASSERT_TRUE(nav.remove("/data/file2.txt"));
    TEST_ASSERT_TRUE(nav.remove("/data/file3.txt"));

    // Verify directory is empty
    files = nav.ls("/data");
    TEST_ASSERT_EQUAL(0, files.size());

    // Clean up
    TEST_ASSERT_TRUE(nav.rmdir("/data"));
}

void test_navigate_create_delete_navigate_cycle(void)
{
    auto m1 = std::make_shared<MockStorageMedia>("", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storageSystemUnderTest->mountMedia(m1, "/");

    DirectoryNavigator nav(storageSystemUnderTest);

    // Create initial structure at root
    TEST_ASSERT_TRUE(nav.mkdir("/test"));
    TEST_ASSERT_TRUE(nav.exists("/test"));

    // Create another directory
    TEST_ASSERT_TRUE(nav.mkdir("/backup"));
    TEST_ASSERT_TRUE(nav.exists("/backup"));

    // Verify navigation
    TEST_ASSERT_TRUE(nav.cd("/test"));
    TEST_ASSERT_EQUAL_STRING("/test", nav.pwd().c_str());

    nav.cd("/backup");
    TEST_ASSERT_EQUAL_STRING("/backup", nav.pwd().c_str());

    // Clean up
    nav.cd("/");
    nav.rmdir("/test");
    nav.rmdir("/backup");
}

void test_overlapping_mountpoints()
{
    // Mount root and /SD pointing to different file systems
    auto rootFs = new MockFileSystem();
    auto sdFs = new MockFileSystem();
    auto root = std::make_shared<MockStorageMedia>("root", true, 1024, 0, 1024, 1024, rootFs);
    auto sd = std::make_shared<MockStorageMedia>("SD", true, 1024, 0, 1024, 1024, sdFs);
    storageSystemUnderTest->mountMedia(root, "/");
    storageSystemUnderTest->mountMedia(sd, "/SD");

    // Write to root and SD separately
    auto f1 = storageSystemUnderTest->open("/rootfile.txt", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(f1.isOpen());
    TEST_ASSERT_TRUE(f1.writeAll("rootdata"));
    f1.close();

    auto f2 = storageSystemUnderTest->open("/SD/sdfile.txt", FILE_MODE_WRITE, true);
    TEST_ASSERT_TRUE(f2.isOpen());
    TEST_ASSERT_TRUE(f2.writeAll("sddata"));
    f2.close();

    // Ensure reading returns correct content from respective filesystems
    ETString r1 = storageSystemUnderTest->open("/rootfile.txt", FILE_MODE_READ).readAll();
    ETString r2 = storageSystemUnderTest->open("/SD/sdfile.txt", FILE_MODE_READ).readAll();
    TEST_ASSERT_EQUAL_STRING("rootdata", r1.c_str());
    TEST_ASSERT_EQUAL_STRING("sddata", r2.c_str());
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
    RUN_TEST(test_open_on_mounted_path_strips_mount_prefix);
    RUN_TEST(test_mountMedia);
    RUN_TEST(test_mountMedia_rejects_null_media);
    RUN_TEST(test_copy_move_remove);
    RUN_TEST(test_media_capacity);
    RUN_TEST(test_unmount_media);
    RUN_TEST(test_exists_with_variation_in_path_formats);

    // StorageSystem + DirectoryNavigator lifecycle tests
    RUN_TEST(test_create_write_read_delete_lifecycle);
    RUN_TEST(test_nested_directory_create_and_navigate);
    RUN_TEST(test_write_across_nested_directories);
    RUN_TEST(test_multiple_files_in_directory_lifecycle);
    RUN_TEST(test_navigate_create_delete_navigate_cycle);

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

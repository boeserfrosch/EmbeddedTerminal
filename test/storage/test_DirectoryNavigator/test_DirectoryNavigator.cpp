// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif

#include <unity.h>
#include "../../../src/DirectoryNavigator.h"
#include "../../../src/interfaces/IFileSystem.h"
#include "../../../src/ETFile.h"
#include "../../Mocks/native/MockStorageMedia.h"
#include "StorageSystem.h"

using namespace EmbeddedTerminal;

// --- DirectoryNavigator tests ---

IStorageSystem *storage;

void setUp(void)
{
    storage = new StorageSystem();
    auto media = new MockStorageMedia("root", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());

    storage->mountMedia(media, "");
}
void tearDown(void)
{
    auto media = storage->media();
    for (auto m : media)
    {
        storage->unmountMedia(m->name());
        delete m;
    }
    delete storage;
}

// Test basic construction
void test_navigator_construction(void)
{
    DirectoryNavigator nav(storage);
    TEST_ASSERT_EQUAL_STRING("/", nav.pwd().c_str());
}

// Test construction with custom root
void test_navigator_custom_root(void)
{
    TEST_ASSERT_TRUE(storage->mkdir("/home"));
    DirectoryNavigator nav(storage, "/home");
    TEST_ASSERT_EQUAL_STRING("/home", nav.pwd().c_str());
}

// Test cd to absolute path
void test_navigator_cd_absolute(void)
{
    storage->mkdir("/home");
    storage->mkdir("/home/user");
    DirectoryNavigator nav(storage);

    bool result = nav.cd("/home/user");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("/home/user", nav.pwd().c_str());
}

// Test cd to relative path
void test_navigator_cd_relative(void)
{
    storage->mkdir("/home");
    storage->mkdir("/home/user");
    DirectoryNavigator nav(storage);

    TEST_ASSERT_TRUE(nav.cd("/home"));
    bool result = nav.cd("user");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("/home/user", nav.pwd().c_str());
}

// Test cd to parent directory
void test_navigator_cd_parent(void)
{
    storage->mkdir("/home");
    storage->mkdir("/home/user");
    DirectoryNavigator nav(storage, "/home/user");

    bool result = nav.cd("..");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("/home", nav.pwd().c_str());
}

// Test cd to non-existent directory
void test_navigator_cd_nonexistent(void)
{
    DirectoryNavigator nav(storage);

    bool result = nav.cd("/nonexistent");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_STRING("/", nav.pwd().c_str()); // Should not change
}

// Test cd to file (should fail)
void test_navigator_cd_to_file(void)
{
    storage->open("/file.txt", "w", true).writeAll("content");
    DirectoryNavigator nav(storage);

    bool result = nav.cd("/file.txt");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_STRING("/", nav.pwd().c_str());
}

// Test exists with absolute path
void test_navigator_exists_absolute(void)
{
    storage->open("/file.txt", "w", true).writeAll("data");
    DirectoryNavigator nav(storage);

    TEST_ASSERT_TRUE(nav.exists("/file.txt"));
    TEST_ASSERT_FALSE(nav.exists("/missing.txt"));
}

// Test exists with relative path
void test_navigator_exists_relative(void)
{
    storage->mkdir("/home");
    storage->open("/home/file.txt", "w", true).writeAll("data");
    DirectoryNavigator nav(storage, "/home");

    TEST_ASSERT_TRUE(nav.exists("file.txt"));
    TEST_ASSERT_FALSE(nav.exists("missing.txt"));
}

// Test ls current directory
void test_navigator_ls_current(void)
{
    storage->mkdir("/home");
    storage->open("/home/a.txt", "w", true).writeAll("a");
    storage->open("/home/b.txt", "w", true).writeAll("b");
    DirectoryNavigator nav(storage, "/home");

    auto files = nav.ls();
    TEST_ASSERT_EQUAL(2, files.size());
}

// Test ls with path
void test_navigator_ls_path(void)
{
    storage->mkdir("/data");
    storage->open("/data/file1.txt", "w", true).writeAll("1");
    storage->open("/data/file2.txt", "w", true).writeAll("2");
    DirectoryNavigator nav(storage);

    auto files = nav.ls("/data");
    TEST_ASSERT_EQUAL(2, files.size());
}

// Test mkdir absolute path
void test_navigator_mkdir_absolute(void)
{
    DirectoryNavigator nav(storage);

    bool result = nav.mkdir("/newdir");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(storage->exists("/newdir"));
}

// Test mkdir relative path
void test_navigator_mkdir_relative(void)
{
    storage->mkdir("/home");
    DirectoryNavigator nav(storage, "/home");

    bool result = nav.mkdir("subdir");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(storage->exists("/home/subdir"));
}

// Test rmdir
void test_navigator_rmdir(void)
{
    storage->mkdir("/tempdir");
    DirectoryNavigator nav(storage);

    bool result = nav.rmdir("/tempdir");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(storage->exists("/tempdir"));
}

// Test remove file
void test_navigator_remove(void)
{
    storage->open("/file.txt", "w", true).writeAll("data");
    DirectoryNavigator nav(storage);

    bool result = nav.remove("/file.txt");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(storage->exists("/file.txt"));
}

// Test isDirectory
void test_navigator_is_directory(void)
{
    storage->mkdir("/dir");
    storage->open("/file.txt", "w", true).writeAll("data");
    DirectoryNavigator nav(storage);

    TEST_ASSERT_TRUE(nav.isDirectory("/dir"));
    TEST_ASSERT_FALSE(nav.isDirectory("/file.txt"));
    TEST_ASSERT_FALSE(nav.isDirectory("/nonexistent"));
}

// Test isEmpty
void test_navigator_is_empty(void)
{
    storage->mkdir("/empty");
    storage->mkdir("/nonempty");
    storage->open("/nonempty/file.txt", "w", true).writeAll("data");
    DirectoryNavigator nav(storage);

    TEST_ASSERT_TRUE(nav.isEmpty("/empty"));
    TEST_ASSERT_FALSE(nav.isEmpty("/nonempty"));
}

// Test pwd with path (resolve path)
void test_navigator_pwd_with_path(void)
{
    storage->mkdir("/home");
    DirectoryNavigator nav(storage, "/home");

    ETString resolved = nav.pwd("subdir");
    TEST_ASSERT_EQUAL_STRING("/home/subdir", resolved.c_str());
}

// Test getStorageSystem accessor
void test_navigator_get_storage_system(void)
{
    DirectoryNavigator nav(storage);

    IStorageSystem *retrieved = nav.getStorageSystem();
    TEST_ASSERT_EQUAL_PTR(storage, retrieved);
}

// Test complex path resolution with .. and .
void test_navigator_complex_path_resolution(void)
{
    TEST_ASSERT_TRUE(storage->mkdir("/a"));
    TEST_ASSERT_TRUE(storage->mkdir("/a/b"));
    TEST_ASSERT_TRUE(storage->mkdir("/a/b/c"));
    DirectoryNavigator nav(storage, "/a/b/c");

    ETString resolved = nav.pwd("../../x");
    TEST_ASSERT_EQUAL_STRING("/a/x", resolved.c_str());
}

// Test cd with . (current directory)
void test_navigator_cd_current(void)
{
    storage->mkdir("/home");
    DirectoryNavigator nav(storage, "/home");

    bool result = nav.cd(".");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("/home", nav.pwd().c_str());
}

// Test path normalization (multiple slashes)
void test_navigator_path_normalization(void)
{
    storage->mkdir("/home");
    DirectoryNavigator nav(storage);

    ETString resolved = nav.pwd("//home//");
    // Should normalize to /home
    TEST_ASSERT_TRUE(resolved.find("/home") != ETString::npos);
}

void test_navigator_exists_returns_false_for_unavailable_path(void)
{
    DirectoryNavigator nav(storage);

    TEST_ASSERT_FALSE(nav.exists("/invalid"));
}

void test_navigator_exists_returns_false_for_unavailable_path_relative(void)
{
    DirectoryNavigator nav(storage, "/home");

    TEST_ASSERT_FALSE(nav.exists("invalid"));
}

void test_navigator_exists_returns_false_for_unavailable_storage(void)
{
    DirectoryNavigator nav(nullptr);

    TEST_ASSERT_FALSE(nav.exists("/any"));
}

// --- DirectoryWalker tests (merged) ---

class NavigatorTest : public EmbeddedTerminal::DirectoryNavigator
{
public:
    NavigatorTest(EmbeddedTerminal::IStorageSystem *fs, const ETString &root = "/") : EmbeddedTerminal::DirectoryNavigator(fs, root) {}
    ETString resolvePathMock(const char *path) const { return resolvePath(path); }
};

void test_resolvePath()
{
    storage->mkdir("/home");
    NavigatorTest nav(storage, "/home");
    // Absolute path
    TEST_ASSERT_EQUAL_STRING("/home", nav.pwd().c_str());
    TEST_ASSERT_EQUAL_STRING("/etc", nav.resolvePathMock("/etc").c_str());
    // Relative path
    TEST_ASSERT_EQUAL_STRING("/home/docs", nav.resolvePathMock("docs").c_str());
    // Empty path
    TEST_ASSERT_EQUAL_STRING("/home", nav.resolvePathMock("").c_str());
}

void test_cd_pwd()
{
    storage->mkdir("/home");
    EmbeddedTerminal::DirectoryNavigator nav(storage, "/home");
    TEST_ASSERT_EQUAL_STRING("/home", nav.pwd().c_str());
    TEST_ASSERT_TRUE(nav.cd("/"));
    TEST_ASSERT_EQUAL_STRING("/", nav.pwd().c_str());
    TEST_ASSERT_FALSE(nav.cd("/notfound"));
    TEST_ASSERT_TRUE(nav.cd("/home"));
    TEST_ASSERT_EQUAL_STRING(nav.pwd().c_str(), "/home");
}

void test_mkdir_rmdir()
{
    storage->mkdir("/home");
    EmbeddedTerminal::DirectoryNavigator nav(storage, "/home");
    TEST_ASSERT_TRUE(nav.mkdir("/newdir"));
    TEST_ASSERT_TRUE(storage->exists("/newdir"));
    TEST_ASSERT_TRUE(nav.rmdir("/newdir"));
    TEST_ASSERT_FALSE(storage->exists("/newdir"));
}

void test_remove()
{
    storage->open("/file.txt", FILE_MODE_WRITE, true).write("data", 4);
    EmbeddedTerminal::DirectoryNavigator nav(storage);
    TEST_ASSERT_TRUE(nav.remove("/file.txt"));
    TEST_ASSERT_FALSE(storage->exists("/file.txt"));
}

void test_cd_pwd_cd_back_to_pwd(void)
{
    storage->mkdir("/folder");
    storage->mkdir("/folder/another");
    storage->mkdir("/folder/another/deeper");
    storage->mkdir("/folder2");
    EmbeddedTerminal::DirectoryNavigator dir(storage, "/");

    TEST_ASSERT_EQUAL_STRING("/", dir.pwd().c_str());
    TEST_ASSERT_TRUE(dir.pwd().isAbsolute());

    // Change to /folder
    Path folder = "folder";
    TEST_ASSERT_FALSE(folder.isAbsolute());
    TEST_ASSERT_TRUE(dir.cd("folder"));
    TEST_ASSERT_TRUE(dir.pwd().isAbsolute());
    TEST_ASSERT_EQUAL_STRING("/folder", dir.pwd().c_str());
    auto pwd = dir.pwd();
    // Change to /folder/another
    TEST_ASSERT_TRUE(dir.cd("another"));
    TEST_ASSERT_EQUAL_STRING("/folder/another", dir.pwd().c_str());

    TEST_ASSERT_TRUE(dir.cd(pwd));
    TEST_ASSERT_EQUAL_STRING(pwd.c_str(), dir.pwd().c_str());

    // Change to /folder/another/deeper
    TEST_ASSERT_TRUE(dir.cd("/folder/another/deeper"));
    TEST_ASSERT_EQUAL_STRING("/folder/another/deeper", dir.pwd().c_str());

    // Go back to /folder/another
    TEST_ASSERT_TRUE(dir.cd(".."));
    TEST_ASSERT_EQUAL_STRING("/folder/another", dir.pwd().c_str());

    // Go back to /folder
    TEST_ASSERT_TRUE(dir.cd(".."));
    TEST_ASSERT_EQUAL_STRING("/folder", dir.pwd().c_str());

    // Change to /folder2 using absolute path
    TEST_ASSERT_TRUE(dir.cd("/folder2"));
    TEST_ASSERT_EQUAL_STRING("/folder2", dir.pwd().c_str());

    // Go back to root
    TEST_ASSERT_TRUE(dir.cd("/"));
    TEST_ASSERT_EQUAL_STRING("/", dir.pwd().c_str());
}

// --- Test runner ---

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
    RUN_TEST(test_navigator_get_storage_system);
    RUN_TEST(test_navigator_complex_path_resolution);
    RUN_TEST(test_navigator_cd_current);
    RUN_TEST(test_navigator_path_normalization);
    RUN_TEST(test_navigator_exists_returns_false_for_unavailable_path);
    RUN_TEST(test_navigator_exists_returns_false_for_unavailable_path_relative);
    RUN_TEST(test_navigator_exists_returns_false_for_unavailable_storage);

    // DirectoryWalker tests
    RUN_TEST(test_resolvePath);
    RUN_TEST(test_cd_pwd);
    RUN_TEST(test_mkdir_rmdir);
    RUN_TEST(test_remove);
    RUN_TEST(test_cd_pwd_cd_back_to_pwd);

    UNITY_END();
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
    process_tests();
    return 0;
}
#endif

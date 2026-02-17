#include "../src/DirectoryNavigator.h"
#include "../src/interfaces/IFileSystem.h"
#include "../src/ETFile.h"
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "../Mocks/MockFileSystem.h"

class NavigatorTest : public EmbeddedTerminal::DirectoryNavigator
{
public:
    NavigatorTest(EmbeddedTerminal::IFileSystem *fs, const ETString &root = "/") : EmbeddedTerminal::DirectoryNavigator(fs, root) {}
    ETString resolvePathMock(const char *path) const { return resolvePath(path); }
};

void test_resolvePath()
{
    MockFileSystem fs;
    fs.createDirectory("/home");
    NavigatorTest nav(&fs, "/home");
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
    MockFileSystem fs;
    fs.createDirectory("/home");
    EmbeddedTerminal::DirectoryNavigator nav(&fs, "/home");
    TEST_ASSERT_EQUAL_STRING("/home", nav.pwd().c_str());
    TEST_ASSERT_TRUE(nav.cd("/"));
    TEST_ASSERT_EQUAL_STRING("/", nav.pwd().c_str());
    TEST_ASSERT_FALSE(nav.cd("/notfound"));
    TEST_ASSERT_TRUE(nav.cd("/home"));
    TEST_ASSERT_EQUAL_STRING(nav.pwd().c_str(), "/home");
}

void test_mkdir_rmdir()
{
    MockFileSystem fs;
    EmbeddedTerminal::DirectoryNavigator nav(&fs);
    TEST_ASSERT_TRUE(nav.mkdir("/newdir"));
    TEST_ASSERT_TRUE(fs.exists("/newdir"));
    TEST_ASSERT_TRUE(nav.rmdir("/newdir"));
    TEST_ASSERT_FALSE(fs.exists("/newdir"));
}

void test_remove()
{
    MockFileSystem fs;
    fs.createFile("/file.txt", "data", 4);
    EmbeddedTerminal::DirectoryNavigator nav(&fs);
    TEST_ASSERT_TRUE(nav.remove("/file.txt"));
    TEST_ASSERT_FALSE(fs.exists("/file.txt"));
}

void test_cd_pwd_cd_back_to_pwd(void)
{
    MockFileSystem FS;
    FS.createDirectory("/folder");
    FS.createDirectory("/folder/another");
    FS.createDirectory("/folder/another/deeper");
    FS.createDirectory("/folder2");
    EmbeddedTerminal::DirectoryNavigator dir(&FS);

    TEST_ASSERT_EQUAL_STRING("/", dir.pwd().c_str());

    // Change to /folder
    TEST_ASSERT_TRUE(dir.cd("folder"));
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

void setUp(void) {}
void tearDown(void) {}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_resolvePath);
    RUN_TEST(test_cd_pwd);
    RUN_TEST(test_mkdir_rmdir);
    RUN_TEST(test_remove);
    RUN_TEST(test_cd_pwd_cd_back_to_pwd);
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

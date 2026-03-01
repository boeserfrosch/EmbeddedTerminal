#include <unity.h>
#include "hal/arduino/ArduinoFileSystem.h"
#include "../../Mocks/MockArduinoFS.h"

using namespace EmbeddedTerminal;

static ArduinoFileSystem *fileSystem;

void setUp(void)
{
    fileSystem = new ArduinoFileSystem(MockArduinoFS);
}

void tearDown(void)
{
    delete fileSystem;
}

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
    file.close();
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
    TEST_ASSERT_FALSE(fileSystem->isEmpty("/someFile"));
    fileSystem->mkdir("/someDir");
    TEST_ASSERT_TRUE(fileSystem->isEmpty("/someDir"));
    fileSystem->open("/someDir/someFile.txt", FILE_MODE_WRITE, true).writeAll("data");
    TEST_ASSERT_FALSE(fileSystem->isEmpty("/someDir"));
    fileSystem->remove("/someDir/someFile.txt");
    TEST_ASSERT_TRUE(fileSystem->isEmpty("/someDir"));
    fileSystem->rmdir("/someDir");
    TEST_ASSERT_FALSE(fileSystem->isEmpty("/someDir"));
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_open_and_write_read_file);
    RUN_TEST(test_exists_and_remove);
    RUN_TEST(test_isEmpty);
    RUN_TEST(test_mkdir_and_rmdir);
    RUN_TEST(test_open_returns_invalid);
    RUN_TEST(test_exists_returns_false);
    RUN_TEST(test_remove_returns_false);
    RUN_TEST(test_mkdir_returns_true);
    RUN_TEST(test_rmdir_returns_false);
    RUN_TEST(test_isDirectory_returns_false);
    RUN_TEST(test_isEmpty_returns_true);
    return UNITY_END();
}

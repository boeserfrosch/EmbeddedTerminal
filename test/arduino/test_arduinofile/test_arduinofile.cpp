#include <unity.h>

#include "hal/arduino/ArduinoFile.h"
#include "../../Mocks/arduino/MockArduinoFS.h"

using namespace EmbeddedTerminal;

static const char *TEST_FILE = "/file.txt";

void setUp(void)
{
    resetMockArduinoFS();
}

void tearDown(void)
{
}

void test_write_and_read_single_byte(void)
{
    ArduinoFile f(MockArduinoFS.open(TEST_FILE, FILE_MODE_WRITE));
    TEST_ASSERT_TRUE(f.isOpen());
    TEST_ASSERT_EQUAL(1, f.write('A'));
    f.close();

    ArduinoFile f2(MockArduinoFS.open(TEST_FILE, FILE_MODE_READ));
    TEST_ASSERT_TRUE(f2.isOpen());
    TEST_ASSERT_EQUAL('A', f2.read());
    f2.close();
}

void test_write_and_read_buffer(void)
{
    const char buf[] = "HelloWorld";
    ArduinoFile f(MockArduinoFS.open(TEST_FILE, FILE_MODE_WRITE));
    size_t written = f.write(buf, sizeof(buf));
    TEST_ASSERT_EQUAL(sizeof(buf), written);
    f.close();

    char readBuf[32] = {0};
    ArduinoFile f2(MockArduinoFS.open(TEST_FILE, FILE_MODE_READ));
    size_t readBytes = f2.read(readBuf, sizeof(buf));
    TEST_ASSERT_EQUAL(sizeof(buf), readBytes);
    TEST_ASSERT_EQUAL_STRING(buf, readBuf);
    f2.close();
}

void test_readAll_and_writeAll(void)
{
    ArduinoFile f(MockArduinoFS.open(TEST_FILE, FILE_MODE_WRITE));
    TEST_ASSERT_TRUE(f.writeAll("Line1\nLine2"));
    f.close();

    ArduinoFile f2(MockArduinoFS.open(TEST_FILE, FILE_MODE_READ));
    ETString content = f2.readAll();
    TEST_ASSERT_TRUE(content.find("Line1") != ETString::npos);
    TEST_ASSERT_TRUE(content.find("Line2") != ETString::npos);
    f2.close();
}

void test_seek_and_position(void)
{
    ArduinoFile f(MockArduinoFS.open(TEST_FILE, FILE_MODE_WRITE));
    f.writeAll("abcdef");
    f.close();

    ArduinoFile f2(MockArduinoFS.open(TEST_FILE, FILE_MODE_READ));
    TEST_ASSERT_TRUE(f2.seek(3));
    TEST_ASSERT_EQUAL(3, f2.position());
    TEST_ASSERT_EQUAL('d', f2.read());
    f2.close();
}

void test_size_and_isEmpty(void)
{
    ArduinoFile f(MockArduinoFS.open(TEST_FILE, FILE_MODE_WRITE));
    f.writeAll("12345");
    f.close();

    ArduinoFile f2(MockArduinoFS.open(TEST_FILE, FILE_MODE_READ));
    TEST_ASSERT_EQUAL(5, f2.size());
    TEST_ASSERT_FALSE(f2.size() == 0);
    f2.close();
}

void test_isOpen_and_close(void)
{
    ArduinoFile f(MockArduinoFS.open(TEST_FILE, FILE_MODE_WRITE));
    TEST_ASSERT_TRUE(f.isOpen());
    f.close();
    TEST_ASSERT_FALSE(f.isOpen());
}

void test_isDirectory(void)
{
    MockArduinoFS.mkdir("/dir");
    ArduinoFile f(MockArduinoFS.open("/dir", FILE_MODE_READ));
    TEST_ASSERT_TRUE(f.isDirectory());
    f.close();
    MockArduinoFS.rmdir("/dir");
}

int process_tests_arduino_file()
{
    UNITY_BEGIN();
    RUN_TEST(test_write_and_read_single_byte);
    RUN_TEST(test_write_and_read_buffer);
    RUN_TEST(test_readAll_and_writeAll);
    RUN_TEST(test_seek_and_position);
    RUN_TEST(test_size_and_isEmpty);
    RUN_TEST(test_isOpen_and_close);
    RUN_TEST(test_isDirectory);
    return UNITY_END();
}

int main()
{
    return process_tests_arduino_file();
}

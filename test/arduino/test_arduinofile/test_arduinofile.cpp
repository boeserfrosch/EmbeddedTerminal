#if defined(ARDUINO)
#include <Arduino.h>
#include <SD.h>
#endif
#include <unity.h>

#include "../src/hal/ArduinoFile.h"

using namespace EmbeddedTerminal;

#if defined(ARDUINO)

static const char *TEST_FILE = "/file.txt";

void setUp(void)
{
    if (!SD.begin())
    {
        TEST_FAIL_MESSAGE("SD init failed, cannot run ArduinoFile tests!");
    }
    if (SD.exists(TEST_FILE))
    {
        SD.remove(TEST_FILE);
    }
}

void tearDown(void)
{
    if (SD.exists(TEST_FILE))
    {
        SD.remove(TEST_FILE);
    }
}

// ---- Tests ----

void test_write_and_read_single_byte(void)
{
    ArduinoFile f(SD.open(TEST_FILE, FILE_MODE_WRITE, true));
    TEST_ASSERT_TRUE(f.isOpen());
    TEST_ASSERT_EQUAL(1, f.write('A'));
    f.close();

    ArduinoFile f2(SD.open(TEST_FILE, FILE_MODE_READ));
    TEST_ASSERT_TRUE(f2.isOpen());
    TEST_ASSERT_EQUAL('A', f2.read());
    f2.close();
}

void test_write_and_read_buffer(void)
{
    const char buf[] = "HelloWorld";
    ArduinoFile f(SD.open(TEST_FILE, FILE_MODE_WRITE, true));
    size_t written = f.write(buf, sizeof(buf));
    TEST_ASSERT_EQUAL(sizeof(buf), written);
    f.close();

    char readBuf[32] = {0};
    ArduinoFile f2(SD.open(TEST_FILE, FILE_MODE_READ));
    size_t readBytes = f2.read(readBuf, sizeof(buf));
    TEST_ASSERT_EQUAL(sizeof(buf), readBytes);
    TEST_ASSERT_EQUAL_STRING(buf, readBuf);
    f2.close();
}

void test_readAll_and_writeAll(void)
{
    ArduinoFile f(SD.open(TEST_FILE, FILE_MODE_WRITE, true));
    TEST_ASSERT_TRUE(f.writeAll("Line1\nLine2"));
    f.close();

    ArduinoFile f2(SD.open(TEST_FILE, FILE_MODE_READ));
    ETString content = f2.readAll();
    TEST_ASSERT_TRUE(content.find("Line1") != ETString::npos);
    TEST_ASSERT_TRUE(content.find("Line2") != ETString::npos);
    f2.close();
}

void test_seek_and_position(void)
{
    ArduinoFile f(SD.open(TEST_FILE, FILE_MODE_WRITE, true));
    f.writeAll("abcdef");
    f.close();

    ArduinoFile f2(SD.open(TEST_FILE, FILE_MODE_READ));
    TEST_ASSERT_TRUE(f2.seek(3));
    TEST_ASSERT_EQUAL(3, f2.position());
    TEST_ASSERT_EQUAL('d', f2.read());
    f2.close();
}

void test_size_and_isEmpty(void)
{
    ArduinoFile f(SD.open(TEST_FILE, FILE_MODE_WRITE, true));
    f.writeAll("12345");
    f.close();

    ArduinoFile f2(SD.open(TEST_FILE, FILE_MODE_READ));
    TEST_ASSERT_EQUAL(5, f2.size());
    TEST_ASSERT_FALSE(f2.size() == 0);
    f2.close();
}

void test_isOpen_and_close(void)
{
    ArduinoFile f(SD.open(TEST_FILE, FILE_MODE_WRITE, true));
    TEST_ASSERT_TRUE(f.isOpen());
    f.close();
    TEST_ASSERT_FALSE(f.isOpen());
}

void test_name_and_path(void)
{
    ArduinoFile f(SD.open(TEST_FILE, FILE_MODE_WRITE, true));
    TEST_ASSERT_TRUE(f.isOpen());
    TEST_ASSERT_TRUE(f.name().find("file.txt") != ETString::npos);
    TEST_ASSERT_EQUAL_STRING(TEST_FILE, f.path().c_str());
    f.close();
}

void test_isDirectory(void)
{
    if (SD.exists("/dir"))
    {
        SD.rmdir("/dir");
    }
    SD.mkdir("/dir");
    ArduinoFile f(SD.open("/dir", FILE_MODE_READ));
    TEST_ASSERT_TRUE(f.isDirectory());
    f.close();
    SD.rmdir("/dir");
}

#else // !ARDUINO (ESP-IDF)

// SD library is not available in ESP-IDF, provide stub implementations
void setUp(void) {}
void tearDown(void) {}

void test_arduinofile_not_available_in_espidf(void)
{
    TEST_IGNORE_MESSAGE("ArduinoFile tests only available on Arduino framework");
}

#endif // ARDUINO

int process_tests_arduino_file()
{
    UNITY_BEGIN();
#if defined(ARDUINO)
    RUN_TEST(test_write_and_read_single_byte);
    RUN_TEST(test_write_and_read_buffer);
    RUN_TEST(test_readAll_and_writeAll);
    RUN_TEST(test_seek_and_position);
    RUN_TEST(test_size_and_isEmpty);
    RUN_TEST(test_isOpen_and_close);
    RUN_TEST(test_name_and_path);
    RUN_TEST(test_isDirectory);
#else
    // ESP-IDF: SD library not available
    RUN_TEST(test_arduinofile_not_available_in_espidf);
#endif
    return UNITY_END();
}

// ---- Arduino entry points ----
#if defined(ARDUINO)
void setup()
{
    delay(2000); // Give serial monitor time to connect
    process_tests_arduino_file();
}
void loop() {}
#else
int main()
{
    return process_tests_arduino_file();
}
#endif

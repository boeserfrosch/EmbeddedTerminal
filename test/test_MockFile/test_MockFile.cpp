#include <unity.h>
#include "../Mocks/native/MockFile.h"

void setUp(void) {}
void tearDown(void) {}

void test_default_constructor()
{
    MockFile f;
    TEST_ASSERT_FALSE(f.isOpen());
    TEST_ASSERT_EQUAL_INT(0, f.size());
    TEST_ASSERT_EQUAL_INT(0, f.position());
    TEST_ASSERT_EQUAL_STRING("", f.readAll().c_str());
}

void test_constructor_with_content()
{
    MockFile f("abc");
    TEST_ASSERT_TRUE(f.isOpen());
    TEST_ASSERT_EQUAL_INT(3, f.size());
    TEST_ASSERT_EQUAL_STRING("abc", f.readAll().c_str());
}

void test_read_single_char()
{
    MockFile f("xyz");
    TEST_ASSERT_EQUAL('x', f.read());
    TEST_ASSERT_EQUAL('y', f.read());
    TEST_ASSERT_EQUAL('z', f.read());
    TEST_ASSERT_EQUAL(-1, f.read());
}

void test_read_buffer()
{
    MockFile f("hello");
    char buf[6] = {0};
    size_t n = f.read(buf, 5);
    TEST_ASSERT_EQUAL_INT(5, n);
    TEST_ASSERT_EQUAL_STRING("hello", buf);
}

void test_write_single_char()
{
    MockFile f;
    f.open_ = true;
    f.write('A');
    TEST_ASSERT_EQUAL_STRING("A", f.readAll().c_str());
}

void test_write_buffer()
{
    MockFile f;
    f.open_ = true;
    const char *data = "test";
    f.write(data, 4);
    TEST_ASSERT_EQUAL_STRING("test", f.readAll().c_str());
}

void test_writeAll()
{
    MockFile f;
    f.open_ = true;
    f.writeAll("foo");
    TEST_ASSERT_EQUAL_STRING("foo", f.readAll().c_str());
}

void test_seek_and_position()
{
    MockFile f("abcdef");
    TEST_ASSERT_TRUE(f.seek(3));
    TEST_ASSERT_EQUAL_INT(3, f.position());
    TEST_ASSERT_FALSE(f.seek(10));
}

void test_close_and_isOpen()
{
    MockFile f("bar");
    f.close();
    TEST_ASSERT_FALSE(f.isOpen());
}

void test_mockSize()
{
    MockFile f("abc", 10);
    TEST_ASSERT_TRUE(f.mockSize_);
    TEST_ASSERT_EQUAL_INT(10, f.size());
}

void test_isDirectory()
{
    MockFile f;
    f.isDirectory_ = true;
    TEST_ASSERT_TRUE(f.isDirectory());
    f.isDirectory_ = false;
    TEST_ASSERT_FALSE(f.isDirectory());
}

int process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_default_constructor);
    RUN_TEST(test_constructor_with_content);
    RUN_TEST(test_read_single_char);
    RUN_TEST(test_read_buffer);
    RUN_TEST(test_write_single_char);
    RUN_TEST(test_write_buffer);
    RUN_TEST(test_writeAll);
    RUN_TEST(test_seek_and_position);
    RUN_TEST(test_close_and_isOpen);
    RUN_TEST(test_mockSize);
    RUN_TEST(test_isDirectory);
    UNITY_END();
    return 0;
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
    return process_tests();
}
#endif

#include "../../src/ETTypes.h"
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif

void test_trim()
{
    ETString s1 = "   hello world   ";
    ETString s2 = "no leading or trailing";
    ETString s3 = "   only leading";
    ETString s4 = "only trailing   ";
    ETString s5 = "   ";

    TEST_ASSERT_EQUAL_STRING(s1.trim().c_str(), "hello world");
    TEST_ASSERT_EQUAL_STRING(s2.trim().c_str(), "no leading or trailing");
    TEST_ASSERT_EQUAL_STRING(s3.trim().c_str(), "only leading");
    TEST_ASSERT_EQUAL_STRING(s4.trim().c_str(), "only trailing");
    TEST_ASSERT_EQUAL_STRING(s5.trim().c_str(), "");
}

void test_cleanupLine()
{
    ETString s1 = "ab\bcdef\b\b"; // should become "acd"
    ETString s2 = "abc\b\b\bdef"; // should become "def"
    ETString s3 = "\b\b\babc";    // should become "abc"
    ETString s4 = "abc\b\b\b";    // should become ""
    ETString s5 = "a\b\b\b\b";    // should become ""

    TEST_ASSERT_EQUAL_STRING(s1.cleanupString().c_str(), "acd");
    TEST_ASSERT_EQUAL_STRING(s2.cleanupString().c_str(), "def");
    TEST_ASSERT_EQUAL_STRING(s3.cleanupString().c_str(), "abc");
    TEST_ASSERT_EQUAL_STRING(s4.cleanupString().c_str(), "");
    TEST_ASSERT_EQUAL_STRING(s5.cleanupString().c_str(), "");

    // Left arrow: ab\x1B[Dcd (move left, insert c, insert d) -> acdb
    ETString s6 = "ab\x1B[Dcd";
    TEST_ASSERT_EQUAL_STRING(s6.cleanupString().c_str(), "acdb");

    // Right arrow: ab\x1B[Dcd\x1B[Cef (move left, insert c, d, move right, insert e, f) -> acdbef
    ETString s7 = "ab\x1B[Dcd\x1B[Cef";
    TEST_ASSERT_EQUAL_STRING(s7.cleanupString().c_str(), "acdbef");

    // Home: abcd\x1B[Hxy (move to start, insert x, y) -> xyabcd
    ETString s8 = "abcd\x1B[Hxy";
    TEST_ASSERT_EQUAL_STRING(s8.cleanupString().c_str(), "xyabcd");

    // End: abcd\x1B[Fxy (move to end, insert x, y) -> abcdxy
    ETString s9 = "abcd\x1B[Fxy";
    TEST_ASSERT_EQUAL_STRING(s9.cleanupString().c_str(), "abcdxy");

    // Combination: abcd\x1B[Hxy\x1B[Fz (move to start, insert x, y, move to end, insert z) -> xyabcdz
    ETString s10 = "abcd\x1B[Hxy\x1B[Fz";
    TEST_ASSERT_EQUAL_STRING(s10.cleanupString().c_str(), "xyabcdz");
}

void test_back_push_pop_back()
{
    ETString s = "abcd";
    TEST_ASSERT_TRUE(s.back() == 'd');
    // Test that back() does not modify the string
    TEST_ASSERT_TRUE(s.back() == 'd');
    s.pop_back();
    TEST_ASSERT_TRUE(s.back() == 'c');
    s.push_back('e');
    TEST_ASSERT_TRUE(s.back() == 'e');
}

void test_substr()
{
    ETString s = "abcdef";
    TEST_ASSERT_EQUAL_STRING("a", s.substr(0, 1).c_str());
    TEST_ASSERT_EQUAL_STRING("b", s.substr(1, 1).c_str());
    TEST_ASSERT_EQUAL_STRING("bcd", s.substr(1, 3).c_str());
    TEST_ASSERT_EQUAL_STRING("bcdef", s.substr(1, 99).c_str());
    TEST_ASSERT_EQUAL_STRING("cdef", s.substr(2).c_str());
}

void test_assign()
{
    ETString s = "ABC";
    TEST_ASSERT_EQUAL_STRING("ABC", s.c_str());
    std::string a = "foo";
    s = a;
    TEST_ASSERT_EQUAL_STRING("foo", s.c_str());
    ETString t(s);
    TEST_ASSERT_EQUAL_STRING("foo", t.c_str());
    const char *b = "bar";
    t = b;
    TEST_ASSERT_EQUAL_STRING("foo", s.c_str());
    TEST_ASSERT_EQUAL_STRING("bar", t.c_str());
    s = t;
    TEST_ASSERT_EQUAL_STRING("bar", s.c_str());
    std::string x = t;
    TEST_ASSERT_EQUAL_STRING("bar", x.c_str());
}

void test_operator_access()
{
    ETString x = "bar";
    char z = x[2];
    TEST_ASSERT_TRUE('r' == z);
}

void test_length()
{
    ETString x = "bar";
    TEST_ASSERT_EQUAL_INT(3, x.length());
}

void test_erase()
{
    ETString s = "ABCDEFG";
    s.erase(1, 1);
    TEST_ASSERT_EQUAL_STRING("ACDEFG", s.c_str());
    s.erase(1, 2);
    TEST_ASSERT_EQUAL_STRING("AEFG", s.c_str());
    s.erase(1);
    TEST_ASSERT_EQUAL_STRING("A", s.c_str());
}

void test_empty()
{
    ETString x = "bar";
    TEST_ASSERT_FALSE(x.empty());
    ETString y = "";
    TEST_ASSERT_TRUE(y.empty());
}

void test_to_lower()
{
    ETString a = "ABC";
    a.toLowerCase();
    TEST_ASSERT_EQUAL_STRING("abc", a.c_str());
    a = "abcd";
    a.toLowerCase();
    TEST_ASSERT_EQUAL_STRING("abcd", a.c_str());
    a = "FOO_BAR& ";
    a.toLowerCase();
    TEST_ASSERT_EQUAL_STRING("foo_bar& ", a.c_str());
}

void test_find()
{
    ETString s = "ABCDEFGHIJKLABCDEF";
    ETString t0 = "DEF";
    auto r0 = s.find(t0);
    TEST_ASSERT_EQUAL_UINT32(3, r0);
    auto r1 = s.find(t0, 5);
    TEST_ASSERT_EQUAL_UINT32(15, r1);
    auto r2 = s.find("FOO");
    TEST_ASSERT_EQUAL_UINT32(ETString::npos, r2);
    auto r2b = s.find("FOO", 2);
    TEST_ASSERT_EQUAL_UINT32(ETString::npos, r2b);
    auto r3 = s.find('B');
    TEST_ASSERT_EQUAL_UINT32(1, r3);
    auto r4 = s.find('B', 3);
    TEST_ASSERT_EQUAL_UINT32(13, r4);
    auto r5 = s.find('Z');
    TEST_ASSERT_EQUAL_UINT32(ETString::npos, r5);
    auto r5b = s.find('Z', 4);
    TEST_ASSERT_EQUAL_UINT32(ETString::npos, r5b);
}

void test_find_last_of()
{
    ETString s = "ABCDFFFGHGHAVK";
    auto r0 = s.find_last_of('C');
    TEST_ASSERT_EQUAL_UINT32(2, r0);
    auto r1 = s.find_last_of('E');
    TEST_ASSERT_EQUAL_UINT32(ETString::npos, r1);
    auto r2 = s.find_last_of('F');
    TEST_ASSERT_EQUAL_UINT32(6, r2);
    auto r3 = s.find_last_of('F', 5);
    TEST_ASSERT_EQUAL_UINT32(5, r3);
    auto r4 = s.find_last_of('F', 4);
    TEST_ASSERT_EQUAL_UINT32(4, r4);
    auto r5 = s.find_last_of('F', 999999);
    TEST_ASSERT_EQUAL_UINT32(6, r5);
    TEST_ASSERT_EQUAL_INT32(9, s.find_last_of("GHA"));
    TEST_ASSERT_EQUAL_INT32(9, s.find_last_of("GH"));
    TEST_ASSERT_EQUAL_INT32(7, s.find_last_of("GH", 8));
}

void test_operators(void)
{
    ETString A = "FOO";
    ETString B = "FOO";
    ETString C = "BAR";
    ETString AC = "FOOBAR";
    ETString AB = "FOOFOO";
    TEST_ASSERT_TRUE(A == B);
    TEST_ASSERT_FALSE(A != B);
    TEST_ASSERT_TRUE((A + C) == AC);
    A += B;
    TEST_ASSERT_TRUE(A == AB);
}

void test_contains(void)
{
    ETString s = "ABCDEFABCDEF";
    ETString c0 = "C";
    ETString c1 = "Z";

    TEST_ASSERT_TRUE(s.contains(c0));
    TEST_ASSERT_TRUE(s.contains(c0.c_str()));
    TEST_ASSERT_TRUE(s.contains('C'));
    TEST_ASSERT_FALSE(s.contains(c1));
    TEST_ASSERT_FALSE(s.contains(c1.c_str()));
    TEST_ASSERT_FALSE(s.contains('Z'));
    TEST_ASSERT_TRUE(s.contains(c0, 3));
    TEST_ASSERT_FALSE(s.contains(c0, 10));
}

void test_endsWith(void)
{
    ETString s = "ABC";
    TEST_ASSERT_TRUE(s.endsWith("BC"));
    TEST_ASSERT_FALSE(s.endsWith("BD"));
    TEST_ASSERT_TRUE(s.endsWith('C'));
    TEST_ASSERT_FALSE(s.endsWith('Z'));
}

void test_startsWith(void)
{
    ETString s = "ABC";
    TEST_ASSERT_TRUE(s.startsWith("AB"));
    TEST_ASSERT_FALSE(s.startsWith("AC"));
    TEST_ASSERT_TRUE(s.startsWith('A'));
    TEST_ASSERT_FALSE(s.startsWith('Z'));
}

void setUp(void) {}
void tearDown(void) {}

void processTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_trim);
    RUN_TEST(test_cleanupLine);
    RUN_TEST(test_back_push_pop_back);
    RUN_TEST(test_substr);
    RUN_TEST(test_assign);
    RUN_TEST(test_find);
    RUN_TEST(test_find_last_of);
    RUN_TEST(test_length);
    RUN_TEST(test_empty);
    RUN_TEST(test_operator_access);
    RUN_TEST(test_erase);
    RUN_TEST(test_to_lower);
    RUN_TEST(test_operators);
    RUN_TEST(test_contains);
    RUN_TEST(test_endsWith);
    RUN_TEST(test_startsWith);
    UNITY_END();
}

#if (defined(ESP_PLATFORM) || defined(ESP32)) && not defined(ARDUINO)
extern "C" void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(4000));
    processTests();
}
#elif defined(ARDUINO)
void setup()
{
    delay(2500);
    processTests();
}
void loop() {}
#else
int main()
{
    processTests();
    return 0;
}
#endif

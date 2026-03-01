
#include <unity.h>
#include "Path.h"

using namespace EmbeddedTerminal;

void setUp(void) {}
void tearDown(void) {}

void test_path_construction()
{
    Path p1("/a/b/c.txt");
    TEST_ASSERT_TRUE(p1.isAbsolute());
    TEST_ASSERT_EQUAL_STRING("c.txt", p1.getName().c_str());
    TEST_ASSERT_EQUAL_STRING("/a/b", p1.getBasePath().c_str());

    Path p2("a/b/c.txt");
    TEST_ASSERT_FALSE(p2.isAbsolute());
    TEST_ASSERT_EQUAL_STRING("c.txt", p2.getName().c_str());
    TEST_ASSERT_EQUAL_STRING("a/b", ETString(p2.getBasePath()).c_str());

    Path p3("");
    TEST_ASSERT_TRUE(p3.isAbsolute());
    TEST_ASSERT_TRUE(p3.isEmpty());

    Path p4("..");
    TEST_ASSERT_FALSE(p4.isAbsolute());
    TEST_ASSERT_EQUAL_STRING("..", p4.getName().c_str());
    TEST_ASSERT_TRUE(p4.getBasePath().isEmpty());
    TEST_ASSERT_EQUAL_STRING("..", ETString(p4).c_str());

    Path p5("/a/b/./c/../d");
    TEST_ASSERT_TRUE(p5.isAbsolute());
    TEST_ASSERT_EQUAL_STRING("d", p5.getName().c_str());
    TEST_ASSERT_EQUAL_STRING("/a/b/d", ETString(p5).c_str()); // Original path should remain unchanged

    // Explicit root path
    Path p6("/");
    TEST_ASSERT_TRUE(p6.isAbsolute());
    TEST_ASSERT_TRUE(p6.isEmpty()); // Root is not considered empty in terms of being a valid path, but it has no name
    TEST_ASSERT_EQUAL_STRING("", p6.getName().c_str());
    TEST_ASSERT_TRUE(p6.getBasePath().isEmpty());
    TEST_ASSERT_EQUAL_STRING("/", ETString(p6).c_str());
}

void test_path_operator_plus()
{
    Path p1("/a/b");
    Path p2("c/d.txt");
    Path p3 = p1 + p2;
    TEST_ASSERT_EQUAL_STRING("/a/b/c/d.txt", ETString(p3).c_str());

    Path p4 = p1 + "e.txt";
    TEST_ASSERT_EQUAL_STRING("/a/b/e.txt", ETString(p4).c_str());
}

void test_path_operator_plus_equals()
{
    Path p1("/a");
    p1 += "b";
    TEST_ASSERT_EQUAL_STRING("/a/b", ETString(p1).c_str());

    p1 += Path("c");
    TEST_ASSERT_EQUAL_STRING("/a/b/c", ETString(p1).c_str());
}

void test_path_normalize()
{
    Path p1("/a/b/./c/../d");
    TEST_ASSERT_EQUAL_STRING("/a/b/d", ETString(p1).c_str());

    Path p2("a/./b/../c");
    TEST_ASSERT_EQUAL_STRING("a/c", ETString(p2).c_str());
}

void test_path_isChildOf_and_isParentOf()
{
    Path parent("/a/b");
    Path child("/a/b/c/d.txt");
    TEST_ASSERT_TRUE(child.isChildOf(parent));
    TEST_ASSERT_TRUE(parent.isParentOf(child));
    TEST_ASSERT_FALSE(parent.isChildOf(child));
    TEST_ASSERT_FALSE(child.isParentOf(parent));
}

void test_path_equality()
{
    TEST_ASSERT_TRUE(Path("/") == Path::root());
    TEST_ASSERT_TRUE(Path("a/./b") == Path("a/b"));
    TEST_ASSERT_TRUE(Path("a/b") != Path("a/c"));
    TEST_ASSERT_FALSE(Path("/a") == Path("a"));
}

void test_path_isFirstOrderChildOf()
{
    Path parent("/a/b");
    Path child1("/a/b/c.txt");
    Path child2("/a/b/c/d.txt");
    TEST_ASSERT_TRUE(child1.isFirstOrderChildOf(parent));
    TEST_ASSERT_FALSE(child2.isFirstOrderChildOf(parent));

    TEST_ASSERT_TRUE(Path("/").isParentOf(Path("/a")));
    TEST_ASSERT_TRUE(Path::root().isParentOf(Path("/a")));
    auto r = Path("/");
    TEST_ASSERT_TRUE(r.isRoot());
    TEST_ASSERT_TRUE(Path("/a").isFirstOrderChildOf(r));
    TEST_ASSERT_TRUE(Path("/a").isFirstOrderChildOf(Path::root()));
    TEST_ASSERT_TRUE(Path("/home/user").isChildOf(Path("/")));
    TEST_ASSERT_TRUE(Path("/home/user").isChildOf(Path::root()));
    TEST_ASSERT_FALSE(Path("/home/user").isFirstOrderChildOf(Path("/")));
    TEST_ASSERT_FALSE(Path("/home/user").isFirstOrderChildOf(Path::root()));
}

void test_path_relativeTo()
{
    Path parent("/a/b");
    Path child("/a/b/c/d.txt");
    Path rel = child.relativeTo(parent);
    TEST_ASSERT_EQUAL_STRING("c/d.txt", ETString(rel).c_str());

    Path unrelated("/x/y/z.txt");
    Path rel2 = unrelated.relativeTo(parent);
    TEST_ASSERT_EQUAL_STRING("/x/y/z.txt", ETString(rel2).c_str());
}

void test_path_c_str_and_conversion()
{
    Path p("/a/b/c.txt");
    const char *cstr = p.c_str();
    TEST_ASSERT_EQUAL_STRING("/a/b/c.txt", cstr); // Path::c_str() return s joined pathParts_ (no leading slash)
    ETString s = p;
    TEST_ASSERT_EQUAL_STRING("/a/b/c.txt", s.c_str());
}

void test_path_backslash_conversion()
{
    Path p("a\\b\\c.txt");
    TEST_ASSERT_EQUAL_STRING("a/b/c.txt", ETString(p).c_str());
}

void test_path_empty_and_root()
{
    Path empty("");
    TEST_ASSERT_TRUE(empty.isAbsolute());
    TEST_ASSERT_TRUE(empty.isEmpty());
    TEST_ASSERT_EQUAL_STRING("/", ETString(empty).c_str());

    Path root("/");
    TEST_ASSERT_TRUE(root.isAbsolute());
    TEST_ASSERT_TRUE(root.isEmpty()); // Root is considered empty in terms of path parts
    TEST_ASSERT_EQUAL_STRING("/", ETString(root).c_str());
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_path_construction);
    RUN_TEST(test_path_operator_plus);
    RUN_TEST(test_path_operator_plus_equals);
    RUN_TEST(test_path_normalize);
    RUN_TEST(test_path_isChildOf_and_isParentOf);
    RUN_TEST(test_path_equality);
    RUN_TEST(test_path_isFirstOrderChildOf);
    RUN_TEST(test_path_relativeTo);
    RUN_TEST(test_path_c_str_and_conversion);
    RUN_TEST(test_path_backslash_conversion);
    RUN_TEST(test_path_empty_and_root);
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
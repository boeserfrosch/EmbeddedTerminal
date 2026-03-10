#include "ShellVariableStore.h"

#include <stdio.h>
#include <string.h>
#include <unity.h>

#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif

using namespace EmbeddedTerminal;

void test_store_set_and_get()
{
    shell_var_store_t store;
    shellVarStoreInit(store);

    TEST_ASSERT_EQUAL(SHELL_OK, shellVarSet(store, "NAME", "alice"));

    const char *value = nullptr;
    TEST_ASSERT_TRUE(shellVarGet(store, "NAME", value));
    TEST_ASSERT_EQUAL_STRING("alice", value);
}

void test_expand_name_and_braced_name()
{
    shell_var_store_t store;
    shellVarStoreInit(store);
    shellVarSet(store, "USER", "bob");

    char out[64];
    TEST_ASSERT_EQUAL(SHELL_OK, shellExpand(store, "$USER ${USER}", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("bob bob", out);
}

void test_expand_default_value()
{
    shell_var_store_t store;
    shellVarStoreInit(store);

    char out[64];
    TEST_ASSERT_EQUAL(SHELL_OK, shellExpand(store, "${MISSING:-fallback}", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("fallback", out);

    shellVarSet(store, "MISSING", "present");
    TEST_ASSERT_EQUAL(SHELL_OK, shellExpand(store, "${MISSING:-fallback}", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("present", out);
}

void test_expand_special_status_argc_positional()
{
    shell_var_store_t store;
    shellVarStoreInit(store);
    shellVarSetLastStatus(store, 42);
    shellVarSetArgCount(store, 3);
    shellVarSetPositional(store, 0, "prog");
    shellVarSetPositional(store, 1, "one");
    shellVarSetPositional(store, 9, "nine");

    char out[128];
    TEST_ASSERT_EQUAL(SHELL_OK, shellExpand(store, "$? $# $0 $1 $9", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("42 3 prog one nine", out);
}

void test_expand_integer_expression()
{
    shell_var_store_t store;
    shellVarStoreInit(store);

    char out[64];
    TEST_ASSERT_EQUAL(SHELL_OK, shellExpand(store, "$((2 + 3 * (4 + 1)))", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("17", out);
}

void test_expand_reports_truncation()
{
    shell_var_store_t store;
    shellVarStoreInit(store);
    shellVarSet(store, "LONG", "abcdefghijklmnopqrstuvwxyz");

    char out[8];
    TEST_ASSERT_EQUAL(SHELL_ERR_TRUNCATED, shellExpand(store, "$LONG", out, sizeof(out)));
    TEST_ASSERT_TRUE(strlen(out) == sizeof(out) - 1);
}

void test_table_full_returns_error()
{
    shell_var_store_t store;
    shellVarStoreInit(store);

    char name[SHELL_VAR_NAME_MAX + 1];
    for (int i = 0; i < SHELL_VAR_MAX; ++i)
    {
        snprintf(name, sizeof(name), "V%d", i);
        TEST_ASSERT_EQUAL(SHELL_OK, shellVarSet(store, name, "x"));
    }

    TEST_ASSERT_EQUAL(SHELL_ERR_TABLE_FULL, shellVarSet(store, "OVER", "y"));
}

void setUp(void) {}
void tearDown(void) {}

void processTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_store_set_and_get);
    RUN_TEST(test_expand_name_and_braced_name);
    RUN_TEST(test_expand_default_value);
    RUN_TEST(test_expand_special_status_argc_positional);
    RUN_TEST(test_expand_integer_expression);
    RUN_TEST(test_expand_reports_truncation);
    RUN_TEST(test_table_full_returns_error);
    UNITY_END();
}

#if (defined(ESP_PLATFORM) || defined(ESP32)) && not defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
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

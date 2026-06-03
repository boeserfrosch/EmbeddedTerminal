// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif

#include "hal/common/ConfigurableGpioPolicy.h"
#include "hal/common/CompileTimeGpioAuth.h"

using namespace EmbeddedTerminal;

void setUp(void) {}
void tearDown(void) {}

void test_policy_default_deny(void)
{
    ConfigurableGpioPolicy policy("test", false, "", "");
    ETString reason;
    TEST_ASSERT_FALSE(policy.isOperationAllowed("GPIO2", GpioOperation::Read, reason));
    TEST_ASSERT_TRUE(reason.find("denied by default") != ETString::npos);
}

void test_policy_allowlist_and_alias_resolution(void)
{
    ConfigurableGpioPolicy policy("test", false, "GPIO2,PA5", "");
    ETString resolved;
    ETString reason;

    TEST_ASSERT_TRUE(policy.resolveIdentifier("2", resolved));
    TEST_ASSERT_TRUE(resolved == "GPIO2");

    TEST_ASSERT_TRUE(policy.resolveIdentifier("pa5", resolved));
    TEST_ASSERT_TRUE(resolved == "PA5");

    TEST_ASSERT_TRUE(policy.resolveIdentifier("GPIO99", resolved));
    TEST_ASSERT_TRUE(resolved == "GPIO99");
    TEST_ASSERT_FALSE(policy.isOperationAllowed(resolved, GpioOperation::Read, reason));
    TEST_ASSERT_TRUE(reason.find("allowlist") != ETString::npos);
}

void test_policy_forced_exclusion_blocks_operations(void)
{
    ConfigurableGpioPolicy policy("test", true, "GPIO2", "GPIO2");
    ETString reason;
    TEST_ASSERT_FALSE(policy.isOperationAllowed("GPIO2", GpioOperation::Write, reason));
    TEST_ASSERT_TRUE(reason.find("forced") != ETString::npos);

    ETString removeReason;
    TEST_ASSERT_FALSE(policy.removeExclusion("GPIO2", removeReason));
    TEST_ASSERT_TRUE(removeReason.find("forced") != ETString::npos);
}

void test_compile_time_auth_hash_validation(void)
{
    ETString hash = CompileTimeGpioAuth::hashPasswordHex("secret");
    CompileTimeGpioAuth auth(hash);

    TEST_ASSERT_TRUE(auth.verifyPassword("secret"));
    TEST_ASSERT_FALSE(auth.verifyPassword("wrong"));
}

void test_policy_runtime_include_overrides_allowlist(void)
{
    ConfigurableGpioPolicy policy("test", false, "GPIO2", "");
    ETString reason;

    TEST_ASSERT_FALSE(policy.isOperationAllowed("GPIO6", GpioOperation::Mode, reason));
    TEST_ASSERT_TRUE(reason.find("allowlist") != ETString::npos);

    GpioExclusionRule includeRule;
    includeRule.pinId = "GPIO6";
    includeRule.denyRead = false;
    includeRule.denyWrite = false;
    includeRule.denyMode = false;
    includeRule.denyInclude = false;
    includeRule.denyExclude = false;

    ETString addReason;
    TEST_ASSERT_TRUE(policy.addExclusion(includeRule, addReason));
    TEST_ASSERT_TRUE(policy.isOperationAllowed("GPIO6", GpioOperation::Mode, reason));

    ETString removeReason;
    TEST_ASSERT_TRUE(policy.removeExclusion("GPIO6", removeReason));
    TEST_ASSERT_FALSE(policy.isOperationAllowed("GPIO6", GpioOperation::Mode, reason));
    TEST_ASSERT_TRUE(reason.find("allowlist") != ETString::npos);
}

void test_policy_allows_all_by_default(void)
{
    ConfigurableGpioPolicy policy("test", true, "", "");
    ETString reason;

    TEST_ASSERT_TRUE(policy.isOperationAllowed("GPIO1", GpioOperation::Read, reason));
    TEST_ASSERT_TRUE(policy.isOperationAllowed("GPIO2", GpioOperation::Write, reason));
    TEST_ASSERT_TRUE(policy.isOperationAllowed("GPIO3", GpioOperation::Mode, reason));
    TEST_ASSERT_TRUE(policy.isOperationAllowed("PA0", GpioOperation::InterruptMode, reason));
}

void test_policy_selective_operation_denial(void)
{
    ConfigurableGpioPolicy policy("test", true, "", "");
    ETString reason;

    GpioExclusionRule denyReadRule;
    denyReadRule.pinId = "GPIO5";
    denyReadRule.denyRead = true;
    denyReadRule.denyWrite = false;
    denyReadRule.denyMode = false;

    ETString addReason;
    TEST_ASSERT_TRUE(policy.addExclusion(denyReadRule, addReason));

    TEST_ASSERT_FALSE(policy.isOperationAllowed("GPIO5", GpioOperation::Read, reason));
    TEST_ASSERT_TRUE(reason.find("denied") != ETString::npos);

    TEST_ASSERT_TRUE(policy.isOperationAllowed("GPIO5", GpioOperation::Write, reason));
    TEST_ASSERT_TRUE(policy.isOperationAllowed("GPIO5", GpioOperation::Mode, reason));
}

void test_policy_multiple_exclusions(void)
{
    ConfigurableGpioPolicy policy("test", true, "", "");
    ETString reason;

    GpioExclusionRule rule1;
    rule1.pinId = "GPIO1";
    rule1.denyWrite = true;

    GpioExclusionRule rule2;
    rule2.pinId = "GPIO2";
    rule2.denyRead = true;

    ETString reason1, reason2;
    TEST_ASSERT_TRUE(policy.addExclusion(rule1, reason1));
    TEST_ASSERT_TRUE(policy.addExclusion(rule2, reason2));

    TEST_ASSERT_TRUE(policy.isOperationAllowed("GPIO1", GpioOperation::Read, reason));
    TEST_ASSERT_FALSE(policy.isOperationAllowed("GPIO1", GpioOperation::Write, reason));

    TEST_ASSERT_FALSE(policy.isOperationAllowed("GPIO2", GpioOperation::Read, reason));
    TEST_ASSERT_TRUE(policy.isOperationAllowed("GPIO2", GpioOperation::Write, reason));
}

void test_policy_alias_case_insensitive_resolution(void)
{
    ConfigurableGpioPolicy policy("test", false, "GPIO2,PA5,PB10", "");
    ETString resolved;

    TEST_ASSERT_TRUE(policy.resolveIdentifier("GPIO2", resolved));
    TEST_ASSERT_TRUE(resolved == "GPIO2");

    TEST_ASSERT_TRUE(policy.resolveIdentifier("gpio2", resolved));
    TEST_ASSERT_TRUE(resolved == "GPIO2");

    TEST_ASSERT_TRUE(policy.resolveIdentifier("PA5", resolved));
    TEST_ASSERT_TRUE(resolved == "PA5");

    TEST_ASSERT_TRUE(policy.resolveIdentifier("pa5", resolved));
    TEST_ASSERT_TRUE(resolved == "PA5");
}

void test_policy_numeric_alias_resolution(void)
{
    ConfigurableGpioPolicy policy("test", false, "GPIO0,GPIO1,GPIO2,GPIO3", "");
    ETString resolved;

    for (int i = 0; i < 4; i++)
    {
        ETString numStr = std::to_string(i).c_str();
        TEST_ASSERT_TRUE(policy.resolveIdentifier(numStr, resolved));

        ETString expected = "GPIO";
        expected += numStr;
        TEST_ASSERT_TRUE(resolved == expected);
    }
}

void test_compile_time_auth_multiple_passwords(void)
{
    ETString hash1 = CompileTimeGpioAuth::hashPasswordHex("password1");
    ETString hash2 = CompileTimeGpioAuth::hashPasswordHex("password2");

    CompileTimeGpioAuth auth1(hash1);
    CompileTimeGpioAuth auth2(hash2);

    TEST_ASSERT_TRUE(auth1.verifyPassword("password1"));
    TEST_ASSERT_FALSE(auth1.verifyPassword("password2"));

    TEST_ASSERT_TRUE(auth2.verifyPassword("password2"));
    TEST_ASSERT_FALSE(auth2.verifyPassword("password1"));
}

void test_policy_empty_allowlist_allows_nothing(void)
{
    ConfigurableGpioPolicy policy("test", false, "", "");
    ETString reason;

    TEST_ASSERT_FALSE(policy.isOperationAllowed("GPIO1", GpioOperation::Read, reason));
    TEST_ASSERT_FALSE(policy.isOperationAllowed("PA0", GpioOperation::Write, reason));
    TEST_ASSERT_FALSE(policy.isOperationAllowed("PB1", GpioOperation::Mode, reason));
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_policy_default_deny);
    RUN_TEST(test_policy_allowlist_and_alias_resolution);
    RUN_TEST(test_policy_forced_exclusion_blocks_operations);
    RUN_TEST(test_compile_time_auth_hash_validation);
    RUN_TEST(test_policy_runtime_include_overrides_allowlist);
    RUN_TEST(test_policy_allows_all_by_default);
    RUN_TEST(test_policy_selective_operation_denial);
    RUN_TEST(test_policy_multiple_exclusions);
    RUN_TEST(test_policy_alias_case_insensitive_resolution);
    RUN_TEST(test_policy_numeric_alias_resolution);
    RUN_TEST(test_compile_time_auth_multiple_passwords);
    RUN_TEST(test_policy_empty_allowlist_allows_nothing);
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
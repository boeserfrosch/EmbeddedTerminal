// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "commands/ping.h"
#include "../Mocks/MockNetworkInterface.h"
#include <string>

void setUp(void) {}
void tearDown(void) {}

/**
 * Test: Empty target (no arguments)
 * Expected: Error message about missing target
 */
void test_ping_empty_target(void)
{
    MockNetworkInterface net;
    EmbeddedTerminal::cmd::ping pingCmd(net);
    ETString keyword = "ping";
    ETString additional = "";
    ETString result = pingCmd.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("Cannot ping without a target") != ETString::npos);
}

/**
 * Test: Whitespace-only arguments
 * Expected: Error message about missing target
 */
void test_ping_whitespace_only(void)
{
    MockNetworkInterface net;
    EmbeddedTerminal::cmd::ping pingCmd(net);
    ETString keyword = "ping";
    ETString additional = "   ";
    ETString result = pingCmd.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("Cannot ping without a target") != ETString::npos);
}

/**
 * Test: Usage message
 * Expected: Usage string with command syntax
 */
void test_ping_usage(void)
{
    MockNetworkInterface net;
    EmbeddedTerminal::cmd::ping pingCmd(net);
    ETString keyword = "ping";
    ETString result = pingCmd.usage(keyword);
    TEST_ASSERT_TRUE(result.find("ping") != ETString::npos);
    TEST_ASSERT_TRUE(result.find("<host>") != ETString::npos);
    TEST_ASSERT_TRUE(result.find("IP address or hostname") != ETString::npos);
}

/**
 * Test: Target extraction (whitespace handling)
 * Expected: Only the first argument is used as target
 */
void test_ping_target_extraction(void)
{
    MockNetworkInterface net;
    EmbeddedTerminal::cmd::ping pingCmd(net);
    ETString keyword = "ping";
    ETString additional = "8.8.8.8 extra arguments";

    // The trigger method should extract just "8.8.8.8"
    // We test by verifying that the response contains "8.8.8.8"
    ETString result = pingCmd.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("8.8.8.8") != ETString::npos);
}

/**
 * Test: Localhost target
 * Expected: Should process localhost appropriately
 */
void test_ping_localhost(void)
{
    MockNetworkInterface net;
    EmbeddedTerminal::cmd::ping pingCmd(net);
    ETString keyword = "ping";
    ETString additional = "localhost";
    ETString result = pingCmd.trigger(keyword, additional);
    // Should contain either "localhost" or a response message
    TEST_ASSERT_TRUE(result.find("localhost") != ETString::npos ||
                     result.find("reachable") != ETString::npos ||
                     result.find("available on this platform") != ETString::npos);
}

/**
 * Test: IPv4 address format
 * Expected: Response should contain the IP address
 */
void test_ping_ipv4_address(void)
{
    MockNetworkInterface net;
    EmbeddedTerminal::cmd::ping pingCmd(net);
    ETString keyword = "ping";
    ETString additional = "192.168.1.1";
    ETString result = pingCmd.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("192.168.1.1") != ETString::npos);
}

/**
 * Test: Response format validation
 * Expected: Response should follow expected format with proper structure
 */
void test_ping_response_format(void)
{
    MockNetworkInterface net;
    EmbeddedTerminal::cmd::ping pingCmd(net);
    ETString keyword = "ping";
    ETString additional = "google.com";
    ETString result = pingCmd.trigger(keyword, additional);

    // Response should contain either:
    // - "not reachable"
    // - "reachable" or "pinged"
    // - "available on this platform"
    // - or platform-specific statistics
    TEST_ASSERT_TRUE(
        result.find("not reachable") != ETString::npos ||
        result.find("reachable") != ETString::npos ||
        result.find("pinged") != ETString::npos ||
        result.find("available") != ETString::npos);
}

/**
 * Platform-specific tests
 */
#if defined(__linux__) || defined(__APPLE__) || defined(_WIN32)
/**
 * Test: Native platform fallback
 * Expected: Should use system ping command (reachability check)
 */
void test_ping_native_platform(void)
{
    MockNetworkInterface net;
    EmbeddedTerminal::cmd::ping pingCmd(net);
    ETString keyword = "ping";
    // 127.0.0.1 should always be reachable on localhost
    ETString additional = "127.0.0.1";
    ETString result = pingCmd.trigger(keyword, additional);

    // Native platform should return reachability message
    TEST_ASSERT_TRUE(
        result.find("reachable") != ETString::npos ||
        result.find("not reachable") != ETString::npos);
}
#endif

#if defined(ESP32)
/**
 * Test: ESP32 platform availability
 * Expected: Should have ping functionality via ESP32Ping library
 */
void test_ping_esp32_available(void)
{
    MockNetworkInterface net;
    EmbeddedTerminal::cmd::ping pingCmd(net);
    ETString keyword = "ping";
    ETString additional = "8.8.8.8";
    ETString result = pingCmd.trigger(keyword, additional);

    // ESP32 implementation should include statistics (times) or reachability message
    TEST_ASSERT_TRUE(
        result.find("pinged") != ETString::npos ||
        result.find("not reachable") != ETString::npos);
}
#endif

#if defined(ESP8266)
/**
 * Test: ESP8266 platform availability
 * Expected: Should have ping functionality via WiFi.ping()
 */
void test_ping_esp8266_available(void)
{
    MockNetworkInterface net;
    EmbeddedTerminal::cmd::ping pingCmd(net);
    ETString keyword = "ping";
    ETString additional = "8.8.8.8";
    ETString result = pingCmd.trigger(keyword, additional);

    // ESP8266 implementation should include response
    TEST_ASSERT_TRUE(
        result.find("pinged") != ETString::npos ||
        result.find("not reachable") != ETString::npos);
}
#endif

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_ping_empty_target);
    RUN_TEST(test_ping_whitespace_only);
    RUN_TEST(test_ping_usage);
    RUN_TEST(test_ping_target_extraction);
    RUN_TEST(test_ping_localhost);
    RUN_TEST(test_ping_ipv4_address);
    RUN_TEST(test_ping_response_format);
#if defined(__linux__) || defined(__APPLE__) || defined(_WIN32)
    RUN_TEST(test_ping_native_platform);
#endif
#if defined(ESP32)
    RUN_TEST(test_ping_esp32_available);
#endif
#if defined(ESP8266)
    RUN_TEST(test_ping_esp8266_available);
#endif
    UNITY_END();
}

#if (defined(ESP_PLATFORM) || defined(ESP32)) && not defined(ARDUINO)
extern "C" void app_main()
{
    process_tests();
}
#else
int main(int argc, char **argv)
{
    process_tests();
    return 0;
}
#endif

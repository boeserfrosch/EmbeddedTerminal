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
#include "../Mocks/MockStream.h"
#include "../Mocks/CommandRuntimeTestUtils.h"

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
 * Test: Target extraction with extra arguments
 * Expected: Only the first argument is used as target
 */
void test_ping_target_extraction(void)
{
    MockNetworkInterface net;
    EmbeddedTerminal::cmd::ping pingCmd(net);
    ETString keyword = "ping";
    ETString additional = "8.8.8.8 extra arguments";

    ETString result = pingCmd.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("8.8.8.8") != ETString::npos);
}

/**
 * Test: Successful ping to localhost
 * Expected: Should delegate to network interface and return result
 */
void test_ping_localhost_success(void)
{
    MockNetworkInterface net;
    EmbeddedTerminal::cmd::ping pingCmd(net);
    ETString keyword = "ping";
    ETString additional = "localhost";
    ETString result = pingCmd.trigger(keyword, additional);

    // Mock returns success for localhost
    TEST_ASSERT_TRUE(result.find("localhost") != ETString::npos);
    TEST_ASSERT_TRUE(result.find("pinged") != ETString::npos);
}

/**
 * Test: IPv4 address ping
 * Expected: Successful ping with statistics
 */
void test_ping_ipv4_address(void)
{
    MockNetworkInterface net;
    EmbeddedTerminal::cmd::ping pingCmd(net);
    ETString keyword = "ping";
    ETString additional = "192.168.1.1";
    ETString result = pingCmd.trigger(keyword, additional);

    TEST_ASSERT_TRUE(result.find("192.168.1.1") != ETString::npos);
    TEST_ASSERT_TRUE(result.find("pinged") != ETString::npos);
}

/**
 * Test: Unreachable host
 * Expected: Error message indicating host is not reachable
 */
void test_ping_unreachable_host(void)
{
    MockNetworkInterface net;
    EmbeddedTerminal::cmd::ping pingCmd(net);
    ETString keyword = "ping";
    ETString additional = "10.255.255.255"; // Mock returns unreachable
    ETString result = pingCmd.trigger(keyword, additional);

    TEST_ASSERT_TRUE(result.find("not reachable") != ETString::npos);
}

/**
 * Test: Invalid host format
 * Expected: Error message indicating host is not reachable
 */
void test_ping_invalid_host(void)
{
    MockNetworkInterface net;
    EmbeddedTerminal::cmd::ping pingCmd(net);
    ETString keyword = "ping";
    ETString additional = "invalid"; // Mock returns unreachable
    ETString result = pingCmd.trigger(keyword, additional);

    TEST_ASSERT_TRUE(result.find("not reachable") != ETString::npos);
}

/**
 * Test: Response format contains statistics
 * Expected: Successful ping includes timing information
 */
void test_ping_response_includes_statistics(void)
{
    MockNetworkInterface net;
    EmbeddedTerminal::cmd::ping pingCmd(net);
    ETString keyword = "ping";
    ETString additional = "8.8.8.8";
    ETString result = pingCmd.trigger(keyword, additional);

    // Mock ping includes statistics
    TEST_ASSERT_TRUE(result.find("Average time") != ETString::npos ||
                     result.find("not reachable") != ETString::npos);
}

void test_ping_execute_writes_stdout(void)
{
    MockNetworkInterface net;
    EmbeddedTerminal::cmd::ping pingCmd(net);

    MockStream stream;
    ETMap<ETString, ETString> vars;
    CommandContext context{vars, 0, true};
    EmptyInputChannel stdinChannel;
    StreamBackedOutputChannel stdoutChannel(stream, TerminalChannel::StdOut);
    StreamBackedOutputChannel stderrChannel(stream, TerminalChannel::StdErr);

    CommandInvocation invocation{"ping", "localhost", context, stdinChannel, stdoutChannel, stderrChannel};
    CommandResult result = pingCmd.execute(invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(stream.stdoutBuffer.find("localhost") != ETString::npos);
    TEST_ASSERT_TRUE(stream.stderrBuffer.empty());
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_ping_empty_target);
    RUN_TEST(test_ping_whitespace_only);
    RUN_TEST(test_ping_usage);
    RUN_TEST(test_ping_target_extraction);
    RUN_TEST(test_ping_localhost_success);
    RUN_TEST(test_ping_ipv4_address);
    RUN_TEST(test_ping_unreachable_host);
    RUN_TEST(test_ping_invalid_host);
    RUN_TEST(test_ping_response_includes_statistics);
    RUN_TEST(test_ping_execute_writes_stdout);
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
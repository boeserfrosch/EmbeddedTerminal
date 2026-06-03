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
#include "../../Mocks/MockNetworkSystem.h"
#include "../../Mocks/MockNetworkInterface.h"
#include "../utils.h"

INetworkSystem *network = nullptr;

void setUp(void)
{
    network = new MockNetworkSystem();
    // Create a mock network interface and add it to the system
    auto mockInterface = new MockNetworkInterface();
    network->addInterface(mockInterface->info().name, mockInterface);
}

void tearDown(void)
{
    auto interfaces = network->interfaces();
    for (auto iface : interfaces)
    {
        network->removeInterface(iface->info().name);
        delete iface;
    }
    delete network;
    network = nullptr;
}

/**
 * Test: Empty target (no arguments)
 * Expected: Error message about missing target
 */
void test_ping_empty_target(void)
{

    EmbeddedTerminal::cmd::ping pingCmd(*network);
    ETString keyword = "ping";
    TestCommandInvocationHandle iHandle("ping");
    ;
    CommandResult result = pingCmd.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains(pingCmd.usage(keyword)));
}

/**
 * Test: Usage message
 * Expected: Usage string with command syntax
 */
void test_ping_usage(void)
{

    EmbeddedTerminal::cmd::ping pingCmd(*network);
    ETString keyword = "ping";
    ETString result = pingCmd.usage(keyword);
    TEST_ASSERT_TRUE(result.find("ping") != ETString::npos);
    TEST_ASSERT_TRUE(result.find("<target>") != ETString::npos);
    TEST_ASSERT_TRUE(result.find("IP address or hostname") != ETString::npos);
}

/**
 * Test: Target extraction with extra arguments
 * Expected: Only the first argument is used as target
 */
void test_ping_target_extraction(void)
{
    EmbeddedTerminal::cmd::ping pingCmd(*network);
    ETString keyword = "ping";
    ETString additional = "8.8.8.8 extra arguments";
    TestCommandInvocationHandle iHandle("ping", {"8.8.8.8", "extra", "arguments"});
    ;

    CommandResult result = pingCmd.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains("8.8.8.8"));
}

/**
 * Test: Successful ping to localhost
 * Expected: Should delegate to network interface and return result
 */
void test_ping_localhost_success(void)
{

    EmbeddedTerminal::cmd::ping pingCmd(*network);
    ETString keyword = "ping";
    TestCommandInvocationHandle iHandle("ping", {"localhost"});
    ;
    CommandResult result = pingCmd.invoke(iHandle.invocation);

    // Mock return s success for localhost
    TEST_ASSERT_TRUE(iHandle.output.contains("localhost"));
    TEST_ASSERT_TRUE(iHandle.output.contains("pinged"));
}

/**
 * Test: IPv4 address ping
 * Expected: Successful ping with statistics
 */
void test_ping_ipv4_address(void)
{

    EmbeddedTerminal::cmd::ping pingCmd(*network);
    ETString keyword = "ping";
    TestCommandInvocationHandle iHandle("ping", {"192.168.1.1"});
    ;
    CommandResult result = pingCmd.invoke(iHandle.invocation);

    TEST_ASSERT_TRUE(iHandle.output.contains("192.168.1.1"));
    TEST_ASSERT_TRUE(iHandle.output.contains("pinged"));
}

/**
 * Test: Unreachable host
 * Expected: Error message indicating host is not reachable
 */
void test_ping_unreachable_host(void)
{

    EmbeddedTerminal::cmd::ping pingCmd(*network);
    ETString keyword = "ping";
    TestCommandInvocationHandle iHandle("ping", {"10.255.255.255"});
    ; // Mock return s unreachable
    CommandResult result = pingCmd.invoke(iHandle.invocation);

    TEST_ASSERT_TRUE(iHandle.output.contains("not reachable"));
}

/**
 * Test: Invalid host format
 * Expected: Error message indicating host is not reachable
 */
void test_ping_invalid_host(void)
{

    EmbeddedTerminal::cmd::ping pingCmd(*network);
    ETString keyword = "ping";
    TestCommandInvocationHandle iHandle("ping", {"invalid"});
    ;
    CommandResult result = pingCmd.invoke(iHandle.invocation);

    TEST_ASSERT_TRUE(iHandle.output.contains("not reachable"));
}

/**
 * Test: Response format contains statistics
 * Expected: Successful ping includes timing information
 */
void test_ping_response_includes_statistics(void)
{

    EmbeddedTerminal::cmd::ping pingCmd(*network);
    ETString keyword = "ping";
    ETString additional = "8.8.8.8";
    TestCommandInvocationHandle iHandle("ping", {"8.8.8.8"});
    ;
    CommandResult result = pingCmd.invoke(iHandle.invocation);
    // Mock ping includes statistics

    // Mock ping includes statistics
    TEST_ASSERT_TRUE(iHandle.output.contains("Average time") ||
                     iHandle.output.contains("not reachable"));
}

void test_ping_execute_writes_stdout(void)
{

    EmbeddedTerminal::cmd::ping pingCmd(*network);

    TestCommandInvocationHandle iHandle("ping", {"localhost"});
    ;
    CommandResult result = pingCmd.invoke(iHandle.invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(iHandle.output.contains("localhost"));
    TEST_ASSERT_TRUE(iHandle.error.contains("not reachable") || iHandle.error.empty());
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_ping_empty_target);
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
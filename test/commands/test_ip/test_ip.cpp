// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#include <unity.h>
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "commands/ip.h"
#include "../../Mocks/MockNetworkSystem.h"
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

void test_ip_valid(void)
{

    network->addInterface("test", new MockNetworkInterface(NetworkInfo("test", "192.168.1.123", "", "", "", true)));
    EmbeddedTerminal::cmd::ip ip(*network);
    ETString keyword = "ip";

    TestCommandInvocationHandle iHandle(keyword);
    ;
    CommandResult result = ip.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains("192.168.1.123"));
    TEST_ASSERT_TRUE(iHandle.output.contains("UP"));
}
void test_no_interface(void)
{
    auto interfaces = network->interfaces();
    for (auto iface : interfaces)
    {
        network->removeInterface(iface->info().name);
        delete iface;
    }

    EmbeddedTerminal::cmd::ip ip(*network);
    ETString keyword = "ip";
    TestCommandInvocationHandle iHandle(keyword);
    ;
    CommandResult result = ip.invoke(iHandle.invocation);

    TEST_ASSERT_TRUE(iHandle.output.contains("No interface available\n"));
}

void test_ip_not_connected(void)
{

    network->addInterface("test", new MockNetworkInterface(NetworkInfo("test", "", "", "", "", false)));
    EmbeddedTerminal::cmd::ip ip(*network);
    ETString keyword = "ip";
    TestCommandInvocationHandle iHandle(keyword);
    ;
    CommandResult result = ip.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains("DOWN"));
}

void test_ip_usage(void)
{

    EmbeddedTerminal::cmd::ip ip(*network);
    ETString keyword = "ip";
    ETString result = ip.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Returns info for all network interfaces") != ETString::npos);
}

void test_ip_single_cases(void)
{

    network->addInterface("foo", new MockNetworkInterface(NetworkInfo("foo", "", "", "", "", false)));
    network->addInterface("bar", new MockNetworkInterface(NetworkInfo("bar", "", "", "", "", false)));
    EmbeddedTerminal::cmd::ip ip(*network);
    ETString keyword = "ip";
    TestCommandInvocationHandle iHandle(keyword, {"-i", "bar"});
    ;
    CommandResult result = ip.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains("bar"));
    TEST_ASSERT_FALSE(iHandle.output.contains("foo"));
    TestCommandInvocationHandle iHandle2(keyword, {"--interface", "foo"});
    CommandResult result2 = ip.invoke(iHandle2.invocation);
    TEST_ASSERT_TRUE(iHandle2.output.contains("foo"));
    TEST_ASSERT_FALSE(iHandle2.output.contains("bar"));
}

void test_multiple_interfaces(void)
{
    network->addInterface("foo", new MockNetworkInterface(NetworkInfo("foo", "", "", "", "", false)));
    network->addInterface("bar", new MockNetworkInterface(NetworkInfo("bar", "", "", "", "", false)));
    EmbeddedTerminal::cmd::ip ip(*network);
    ETString keyword = "ip";
    ETString additional = "-i bar -i foo";
    TestCommandInvocationHandle iHandle(keyword, {"-i", "bar", "-i", "foo"});
    ;
    CommandResult result = ip.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains("bar"));
    TEST_ASSERT_TRUE(iHandle.output.contains("foo"));
}

void test_unknown_iface(void)
{

    network->addInterface("foo", new MockNetworkInterface(NetworkInfo("foo", "", "", "", "", false)));
    network->addInterface("bar", new MockNetworkInterface(NetworkInfo("bar", "", "", "", "", false)));
    EmbeddedTerminal::cmd::ip ip(*network);
    ETString keyword = "ip";
    TestCommandInvocationHandle iHandle(keyword, {"-i", "baz"});
    ;
    CommandResult result = ip.invoke(iHandle.invocation);
    TEST_ASSERT_TRUE(iHandle.output.contains("Unknown interface: baz"));
}

void test_ip_execute_writes_stdout(void)
{

    network->addInterface("test", new MockNetworkInterface(NetworkInfo("test", "192.168.1.123", "", "", "", true)));
    EmbeddedTerminal::cmd::ip ip(*network);

    TestCommandInvocationHandle iHandle("ip");
    ;
    CommandResult result = ip.invoke(iHandle.invocation);

    TEST_ASSERT_EQUAL(0, result.exitCode);
    TEST_ASSERT_TRUE(iHandle.output.contains("192.168.1.123"));
    TEST_ASSERT_TRUE(iHandle.error.empty());
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_ip_valid);
    RUN_TEST(test_ip_not_connected);
    RUN_TEST(test_ip_usage);
    RUN_TEST(test_ip_single_cases);
    RUN_TEST(test_no_interface);
    RUN_TEST(test_unknown_iface);
    RUN_TEST(test_ip_execute_writes_stdout);
    RUN_TEST(test_multiple_interfaces);
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

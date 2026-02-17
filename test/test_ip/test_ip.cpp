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
#include "../Mocks/MockNetworkInterface.h"
#include <string>

void setUp(void) {}
void tearDown(void) {}

void test_ip_valid(void)
{
    MockNetworkInterface net;
    net.addInterface("test", "192.168.1.123", "", "", "", true);
    EmbeddedTerminal::cmd::ip ip(net);
    ETString keyword = "ip";
    ETString additional = "";
    ETString result = ip.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("192.168.1.123") != ETString::npos);
    TEST_ASSERT_TRUE(result.find("UP") != ETString::npos);
}
void test_no_interface(void)
{
    MockNetworkInterface net;
    EmbeddedTerminal::cmd::ip ip(net);
    ETString keyword = "ip";
    ETString additional = "";
    ETString result = ip.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("No interface available\n") != ETString::npos);
}

void test_ip_not_connected(void)
{
    MockNetworkInterface net;
    net.addInterface("test", "", "", "", "", false);
    EmbeddedTerminal::cmd::ip ip(net);
    ETString keyword = "ip";
    ETString additional = "";
    ETString result = ip.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("DOWN") != ETString::npos);
}

void test_ip_usage(void)
{
    MockNetworkInterface net;
    EmbeddedTerminal::cmd::ip ip(net);
    ETString keyword = "ip";
    ETString result = ip.usage(keyword);
    TEST_ASSERT_TRUE(result.find("Returns info for all network interfaces") != ETString::npos);
}

void test_ip_single_cases(void)
{
    MockNetworkInterface net;
    net.addInterface("foo", "", "", "", "", false);
    net.addInterface("bar", "", "", "", "", false);
    EmbeddedTerminal::cmd::ip ip(net);
    ETString keyword = "ip";
    ETString additional = "bar";
    ETString result = ip.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("bar") != ETString::npos);
    TEST_ASSERT_TRUE(result.find("foo") == ETString::npos);
    ETString additional2 = "foo";
    ETString result2 = ip.trigger(keyword, additional2);
    TEST_ASSERT_TRUE(result2.find("foo") != ETString::npos);
    TEST_ASSERT_TRUE(result2.find("bar") == ETString::npos);
}

void test_unknown_iface(void)
{
    MockNetworkInterface net;
    net.addInterface("foo", "", "", "", "", false);
    net.addInterface("bar", "", "", "", "", false);
    EmbeddedTerminal::cmd::ip ip(net);
    ETString keyword = "ip";
    ETString additional = "baz";
    ETString result = ip.trigger(keyword, additional);
    TEST_ASSERT_TRUE(result.find("Unknown interface\n") != ETString::npos);
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

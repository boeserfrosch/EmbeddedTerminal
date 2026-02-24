// Platform conditional includes
#if defined(ARDUINO) //|| defined(ESP_PLATFORM)
#include <Arduino.h>
#endif
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif
#include "OptionParser.h"
#include "../Mocks/MockFileSystem.h"
#include "../Mocks/CommandRuntimeTestUtils.h"
#include "DirectoryNavigator.h"
#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

void test_option_parser_simple(void)
{
    EmbeddedTerminal::OptionParser parser;
    parser.addOption("-n", "--lines", "Number of lines to display", true);

    auto result = parser.parse("-n 5 file.txt");
    TEST_ASSERT_TRUE(result.success);
    TEST_ASSERT_TRUE(result.options.find("--lines") != result.options.end());
    TEST_ASSERT_EQUAL_STRING("5", result.options["--lines"].c_str());
    TEST_ASSERT_EQUAL_STRING("file.txt", result.remainingArguments.c_str());
}

void test_option_parser_with_multiple_options(void)
{
    EmbeddedTerminal::OptionParser parser;
    parser.addOption("-n", "--lines", "Number of lines to display", true);
    parser.addOption("-f", "--force", "Force display even if file is large");

    auto result = parser.parse("-n 5 -f file.txt");
    TEST_ASSERT_TRUE(result.success);
    TEST_ASSERT_TRUE(result.options.find("--lines") != result.options.end());
    TEST_ASSERT_TRUE(result.options.find("--force") != result.options.end());
    TEST_ASSERT_EQUAL_STRING("5", result.options["--lines"].c_str());
    TEST_ASSERT_EQUAL_STRING("true", result.options["--force"].c_str());
    TEST_ASSERT_EQUAL_STRING("file.txt", result.remainingArguments.c_str());
}

void test_option_parser_with_invalid_options(void)
{
    EmbeddedTerminal::OptionParser parser;
    parser.addOption("-n", "--lines", "Number of lines to display", true);

    auto result = parser.parse("-x 5 file.txt");
    TEST_ASSERT_TRUE(result.success);
    TEST_ASSERT_TRUE(result.options.find("--lines") == result.options.end());
    TEST_ASSERT_EQUAL_STRING("-x 5 file.txt", result.remainingArguments.c_str());
}

void test_option_parser_with_missing_required_options(void)
{
    EmbeddedTerminal::OptionParser parser;
    parser.addOption("-n", "--lines", "Number of lines to display", true);
    parser.addRequiredRemainingArgument("file");

    auto result = parser.parse("file.txt -n");
    TEST_ASSERT_FALSE(result.success);
    TEST_ASSERT_TRUE(result.errorMessage.find("requires a value") != ETString::npos);
}

void test_option_parser_with_extra_arguments(void)
{
    EmbeddedTerminal::OptionParser parser;
    parser.addOption("-n", "--lines", "Number of lines to display", true);

    auto result = parser.parse("-n 5 file.txt extra_arg");
    TEST_ASSERT_TRUE(result.success);
    TEST_ASSERT_TRUE(result.options.find("--lines") != result.options.end());
    TEST_ASSERT_EQUAL_STRING("5", result.options["--lines"].c_str());
    TEST_ASSERT_EQUAL_STRING("file.txt extra_arg", result.remainingArguments.c_str());
}

void test_option_parser_with_quoted_value(void)
{
    EmbeddedTerminal::OptionParser parser;
    parser.addOption("-n", "--name", "Name with spaces", true);

    auto result = parser.parse("--name \"John Doe\" file.txt");
    TEST_ASSERT_TRUE(result.success);
    TEST_ASSERT_TRUE(result.options.find("--name") != result.options.end());
    TEST_ASSERT_EQUAL_STRING("John Doe", result.options["--name"].c_str());
    TEST_ASSERT_EQUAL_STRING("file.txt", result.remainingArguments.c_str());
}

void test_option_parser_with_unmatched_quote(void)
{
    EmbeddedTerminal::OptionParser parser;
    parser.addOption("-n", "--name", "Name with spaces", true);

    auto result = parser.parse("--name \"John Doe file.txt");
    TEST_ASSERT_FALSE(result.success);
    TEST_ASSERT_TRUE(result.errorMessage.find("Unmatched quote") != ETString::npos);
}

void test_option_parser_with_required_remaining_arguments(void)
{
    EmbeddedTerminal::OptionParser parser;
    parser.addOption("-n", "--lines", "Number of lines to display", true);
    parser.addRequiredRemainingArgument("file");

    auto result = parser.parse("-n 5");
    TEST_ASSERT_FALSE(result.success);
    TEST_ASSERT_TRUE(result.errorMessage.find("Missing required argument") != ETString::npos);
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_option_parser_simple);
    RUN_TEST(test_option_parser_with_multiple_options);
    RUN_TEST(test_option_parser_with_invalid_options);
    RUN_TEST(test_option_parser_with_missing_required_options);
    RUN_TEST(test_option_parser_with_extra_arguments);
    RUN_TEST(test_option_parser_with_quoted_value);
    RUN_TEST(test_option_parser_with_unmatched_quote);
    RUN_TEST(test_option_parser_with_required_remaining_arguments);
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
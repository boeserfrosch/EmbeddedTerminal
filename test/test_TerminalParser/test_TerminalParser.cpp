// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#endif

#include <unity.h>
#include "TerminalParser.h"

using namespace EmbeddedTerminal;

void setUp(void) {}
void tearDown(void) {}

void test_parser_parses_simple_command(void)
{
    TerminalParser parser;
    ETVector<token_t> tokens;
    LexerError lexerError = LexerError::NONE;

    TEST_ASSERT_TRUE(parser.tokenizeLine("echo hi", tokens, lexerError));
    ParsedAst ast;
    TEST_ASSERT_TRUE(parser.parseTokens(tokens, ast));

    TEST_ASSERT_FALSE(ast.isForLoop);
    TEST_ASSERT_EQUAL(1, ast.chain.segments.size());
    TEST_ASSERT_EQUAL_STRING("echo", ast.chain.segments[0].command.keywords[0].c_str());
    TEST_ASSERT_EQUAL_STRING("hi", ast.chain.segments[0].command.arguments[0].c_str());
}

void test_parser_parses_chain_operators(void)
{
    TerminalParser parser;
    ETVector<token_t> tokens;
    LexerError lexerError = LexerError::NONE;

    TEST_ASSERT_TRUE(parser.tokenizeLine("a && b || c ; d", tokens, lexerError));
    ParsedAst ast;
    TEST_ASSERT_TRUE(parser.parseTokens(tokens, ast));

    TEST_ASSERT_EQUAL(4, ast.chain.segments.size());
    TEST_ASSERT_EQUAL(ChainCondition::Always, ast.chain.segments[0].condition);
    TEST_ASSERT_EQUAL(ChainCondition::OnSuccess, ast.chain.segments[1].condition);
    TEST_ASSERT_EQUAL(ChainCondition::OnFailure, ast.chain.segments[2].condition);
    TEST_ASSERT_EQUAL(ChainCondition::Always, ast.chain.segments[3].condition);
}

void test_parser_parses_for_loop(void)
{
    TerminalParser parser;
    ETVector<token_t> tokens;
    LexerError lexerError = LexerError::NONE;

    TEST_ASSERT_TRUE(parser.tokenizeLine("for i in one two; do echo $i; done", tokens, lexerError));
    ParsedAst ast;
    TEST_ASSERT_TRUE(parser.parseTokens(tokens, ast));

    TEST_ASSERT_TRUE(ast.isForLoop);
    TEST_ASSERT_EQUAL_STRING("i", ast.forLoop.variable.c_str());
    TEST_ASSERT_EQUAL(2, ast.forLoop.values.size());
    TEST_ASSERT_EQUAL_STRING("one", ast.forLoop.values[0].c_str());
    TEST_ASSERT_EQUAL_STRING("two", ast.forLoop.values[1].c_str());
}

void test_parser_rejects_invalid_chain(void)
{
    TerminalParser parser;
    ETVector<token_t> tokens;
    LexerError lexerError = LexerError::NONE;

    TEST_ASSERT_TRUE(parser.tokenizeLine("echo a &&", tokens, lexerError));
    ParsedAst ast;
    TEST_ASSERT_FALSE(parser.parseTokens(tokens, ast));
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_parser_parses_simple_command);
    RUN_TEST(test_parser_parses_chain_operators);
    RUN_TEST(test_parser_parses_for_loop);
    RUN_TEST(test_parser_rejects_invalid_chain);
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

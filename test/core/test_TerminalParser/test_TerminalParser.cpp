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
#include "TerminalTokenizer.h"

using namespace EmbeddedTerminal;

void setUp(void) {}
void tearDown(void) {}

void test_parser_parses_simple_command(void)
{
    TerminalTokenizer tokenizer;
    TerminalParser parser;
    ETVector<token_t> tokens;
    LexerError lexerError = LexerError::NONE;

    TEST_ASSERT_TRUE(tokenizer.tokenizeLine("echo hi", tokens, lexerError));
    ParsedAst ast;
    TEST_ASSERT_TRUE(parser.parseTokens(tokens, ast));

    TEST_ASSERT_FALSE(ast.isForLoop);
    TEST_ASSERT_EQUAL(1, ast.chain.segments.size());
    TEST_ASSERT_EQUAL_STRING("echo", ast.chain.segments[0].command.keywords[0].c_str());
    TEST_ASSERT_EQUAL_STRING("hi", ast.chain.segments[0].command.arguments[0].c_str());
}

void test_parser_parses_chain_operators(void)
{
    TerminalTokenizer tokenizer;
    TerminalParser parser;
    ETVector<token_t> tokens;
    LexerError lexerError = LexerError::NONE;

    TEST_ASSERT_TRUE(tokenizer.tokenizeLine("a && b || c ; d", tokens, lexerError));
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
    TerminalTokenizer tokenizer;
    TerminalParser parser;
    ETVector<token_t> tokens;
    LexerError lexerError = LexerError::NONE;

    TEST_ASSERT_TRUE(tokenizer.tokenizeLine("for i in one two; do echo $i; done", tokens, lexerError));
    ParsedAst ast;
    TEST_ASSERT_TRUE(parser.parseTokens(tokens, ast));

    TEST_ASSERT_TRUE(ast.isForLoop);
    TEST_ASSERT_EQUAL_STRING("i", ast.forLoop.variable.c_str());
    TEST_ASSERT_EQUAL(2, ast.forLoop.values.size());
    TEST_ASSERT_EQUAL_STRING("one", ast.forLoop.values[0].c_str());
    TEST_ASSERT_EQUAL_STRING("two", ast.forLoop.values[1].c_str());
}

void test_parser_parses_while_true_loop(void)
{
    TerminalTokenizer tokenizer;
    TerminalParser parser;
    ETVector<token_t> tokens;
    LexerError lexerError = LexerError::NONE;

    TEST_ASSERT_TRUE(tokenizer.tokenizeLine("while true; do echo hi; done", tokens, lexerError));
    ParsedAst ast;
    TEST_ASSERT_TRUE(parser.parseTokens(tokens, ast));

    TEST_ASSERT_TRUE(ast.isWhileLoop);
    TEST_ASSERT_TRUE(ast.whileLoop.hasLiteralCondition);
    TEST_ASSERT_TRUE(ast.whileLoop.literalCondition);
    TEST_ASSERT_EQUAL(1, ast.whileLoop.body.segments.size());
}

void test_parser_parses_while_condition_statement(void)
{
    TerminalTokenizer tokenizer;
    TerminalParser parser;
    ETVector<token_t> tokens;
    LexerError lexerError = LexerError::NONE;

    TEST_ASSERT_TRUE(tokenizer.tokenizeLine("while echo ok; do echo hi; done", tokens, lexerError));
    ParsedAst ast;
    TEST_ASSERT_TRUE(parser.parseTokens(tokens, ast));

    TEST_ASSERT_TRUE(ast.isWhileLoop);
    TEST_ASSERT_FALSE(ast.whileLoop.hasLiteralCondition);
    TEST_ASSERT_EQUAL(1, ast.whileLoop.condition.segments.size());
    TEST_ASSERT_EQUAL_STRING("echo", ast.whileLoop.condition.segments[0].command.keywords[0].c_str());
}

void test_parser_parses_if_then_else_block(void)
{
    TerminalTokenizer tokenizer;
    TerminalParser parser;
    ETVector<token_t> tokens;
    LexerError lexerError = LexerError::NONE;

    TEST_ASSERT_TRUE(tokenizer.tokenizeLine("if echo ok; then echo yes; else echo no; fi", tokens, lexerError));
    ParsedAst ast;
    TEST_ASSERT_TRUE(parser.parseTokens(tokens, ast));

    TEST_ASSERT_TRUE(ast.isIfBlock);
    TEST_ASSERT_EQUAL(1, ast.ifBlock.condition.segments.size());
    TEST_ASSERT_EQUAL(1, ast.ifBlock.thenBody.segments.size());
    TEST_ASSERT_TRUE(ast.ifBlock.hasElse);
    TEST_ASSERT_EQUAL(1, ast.ifBlock.elseBody.segments.size());
    TEST_ASSERT_EQUAL_STRING("echo", ast.ifBlock.thenBody.segments[0].command.keywords[0].c_str());
    TEST_ASSERT_EQUAL_STRING("yes", ast.ifBlock.thenBody.segments[0].command.arguments[0].c_str());
}

void test_parser_parses_if_with_elif(void)
{
    TerminalTokenizer tokenizer;
    TerminalParser parser;
    ETVector<token_t> tokens;
    LexerError lexerError = LexerError::NONE;

    TEST_ASSERT_TRUE(tokenizer.tokenizeLine("if fail; then echo no; elif echo ok; then echo yes; else echo fallback; fi", tokens, lexerError));
    ParsedAst ast;
    TEST_ASSERT_TRUE(parser.parseTokens(tokens, ast));

    TEST_ASSERT_TRUE(ast.isIfBlock);
    TEST_ASSERT_EQUAL(1, ast.ifBlock.condition.segments.size());
    TEST_ASSERT_EQUAL(1, ast.ifBlock.thenBody.segments.size());
    TEST_ASSERT_EQUAL(1, ast.ifBlock.elifConditions.size());
    TEST_ASSERT_EQUAL(1, ast.ifBlock.elifBodies.size());
    TEST_ASSERT_TRUE(ast.ifBlock.hasElse);
    TEST_ASSERT_EQUAL_STRING("echo", ast.ifBlock.elifBodies[0].segments[0].command.keywords[0].c_str());
    TEST_ASSERT_EQUAL_STRING("yes", ast.ifBlock.elifBodies[0].segments[0].command.arguments[0].c_str());
}

void test_parser_extracts_function_def(void)
{
    TerminalTokenizer tokenizer;
    TerminalParser parser;
    ETVector<token_t> tokens;
    LexerError lexerError = LexerError::NONE;

    TEST_ASSERT_TRUE(tokenizer.tokenizeLine("function blink; do echo hi; done; echo after", tokens, lexerError));

    ETVector<ParsedFunctionDef> defs;
    TEST_ASSERT_TRUE(parser.extractFunctionDefs(tokens, defs));

    TEST_ASSERT_EQUAL(1, defs.size());
    TEST_ASSERT_EQUAL_STRING("blink", defs[0].name.c_str());
    TEST_ASSERT_EQUAL(1, defs[0].body.segments.size());
    TEST_ASSERT_EQUAL_STRING("echo", defs[0].body.segments[0].command.keywords[0].c_str());
    TEST_ASSERT_EQUAL_STRING("hi", defs[0].body.segments[0].command.arguments[0].c_str());

    ParsedAst ast;
    TEST_ASSERT_TRUE(parser.parseTokens(tokens, ast));
    TEST_ASSERT_EQUAL(1, ast.chain.segments.size());
    TEST_ASSERT_EQUAL_STRING("echo", ast.chain.segments[0].command.keywords[0].c_str());
    TEST_ASSERT_EQUAL_STRING("after", ast.chain.segments[0].command.arguments[0].c_str());
}

void test_parser_rejects_if_without_fi(void)
{
    TerminalTokenizer tokenizer;
    TerminalParser parser;
    ETVector<token_t> tokens;
    LexerError lexerError = LexerError::NONE;

    TEST_ASSERT_TRUE(tokenizer.tokenizeLine("if echo ok; then echo yes", tokens, lexerError));
    ParsedAst ast;
    TEST_ASSERT_FALSE(parser.parseTokens(tokens, ast));
}

void test_parser_rejects_invalid_chain(void)
{
    TerminalTokenizer tokenizer;
    TerminalParser parser;
    ETVector<token_t> tokens;
    LexerError lexerError = LexerError::NONE;

    TEST_ASSERT_TRUE(tokenizer.tokenizeLine("echo a &&", tokens, lexerError));
    ParsedAst ast;
    TEST_ASSERT_FALSE(parser.parseTokens(tokens, ast));
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_parser_parses_simple_command);
    RUN_TEST(test_parser_parses_chain_operators);
    RUN_TEST(test_parser_parses_for_loop);
    RUN_TEST(test_parser_parses_while_true_loop);
    RUN_TEST(test_parser_parses_while_condition_statement);
    RUN_TEST(test_parser_parses_if_then_else_block);
    RUN_TEST(test_parser_parses_if_with_elif);
    RUN_TEST(test_parser_extracts_function_def);
    RUN_TEST(test_parser_rejects_if_without_fi);
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

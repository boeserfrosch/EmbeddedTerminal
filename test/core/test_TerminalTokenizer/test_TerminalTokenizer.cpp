#include "../../../src/TerminalTokenizer.h"
#include <unity.h>

#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif

using namespace EmbeddedTerminal;

void test_lex_emits_expected_operator_tokens()
{
    TerminalTokenizer tokenizer;
    ETVector<token_t> tokens;
    LexerError lexerError = LexerError::NONE;

    bool result = tokenizer.tokenizeLine("| > < >> ; && ||", tokens, lexerError);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(LexerError::NONE, lexerError);
    TEST_ASSERT_EQUAL_UINT32(8, tokens.size());

    TEST_ASSERT_EQUAL(TokenType::PIPE, tokens[0].type);
    TEST_ASSERT_EQUAL(TokenType::REDIR_OUT, tokens[1].type);
    TEST_ASSERT_EQUAL(TokenType::REDIR_IN, tokens[2].type);
    TEST_ASSERT_EQUAL(TokenType::REDIR_APPEND, tokens[3].type);
    TEST_ASSERT_EQUAL(TokenType::SEMI, tokens[4].type);
    TEST_ASSERT_EQUAL(TokenType::AND_AND, tokens[5].type);
    TEST_ASSERT_EQUAL(TokenType::OR_OR, tokens[6].type);
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[7].type);
}

void test_lex_handles_single_double_and_unquoted_words()
{
    TerminalTokenizer tokenizer;
    ETVector<token_t> tokens;
    LexerError lexerError = LexerError::NONE;

    bool result = tokenizer.tokenizeLine("echo 'single quoted' \"double quoted\" plain", tokens, lexerError);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(LexerError::NONE, lexerError);
    TEST_ASSERT_EQUAL_UINT32(5, tokens.size());

    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[0].type);
    TEST_ASSERT_EQUAL_STRING("echo", tokens[0].text.c_str());
    TEST_ASSERT_FALSE(tokens[0].singleQuoted);
    TEST_ASSERT_FALSE(tokens[0].doubleQuoted);

    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[1].type);
    TEST_ASSERT_EQUAL_STRING("single quoted", tokens[1].text.c_str());
    TEST_ASSERT_TRUE(tokens[1].singleQuoted);
    TEST_ASSERT_FALSE(tokens[1].doubleQuoted);

    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[2].type);
    TEST_ASSERT_EQUAL_STRING("double quoted", tokens[2].text.c_str());
    TEST_ASSERT_FALSE(tokens[2].singleQuoted);
    TEST_ASSERT_TRUE(tokens[2].doubleQuoted);

    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[3].type);
    TEST_ASSERT_EQUAL_STRING("plain", tokens[3].text.c_str());
    TEST_ASSERT_FALSE(tokens[3].singleQuoted);
    TEST_ASSERT_FALSE(tokens[3].doubleQuoted);

    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[4].type);
}

void test_lex_emits_newline_token()
{
    TerminalTokenizer tokenizer;
    ETVector<token_t> tokens;
    LexerError lexerError = LexerError::NONE;

    bool result = tokenizer.tokenizeLine("echo hi\nls", tokens, lexerError);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(LexerError::NONE, lexerError);
    TEST_ASSERT_EQUAL_UINT32(5, tokens.size());

    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[0].type);
    TEST_ASSERT_EQUAL_STRING("echo", tokens[0].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[1].type);
    TEST_ASSERT_EQUAL_STRING("hi", tokens[1].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::NEWLINE, tokens[2].type);
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[3].type);
    TEST_ASSERT_EQUAL_STRING("ls", tokens[3].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[4].type);
}

void test_lex_allows_large_token_sequences_without_exhaustion()
{
    TerminalTokenizer tokenizer;
    ETVector<token_t> tokens;
    LexerError lexerError = LexerError::NONE;

    bool result = tokenizer.tokenizeLine("echo hello", tokens, lexerError);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(LexerError::NONE, lexerError);
    TEST_ASSERT_EQUAL_UINT32(3, tokens.size());
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[0].type);
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[1].type);
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[2].type);
}

void setUp(void) {}
void tearDown(void) {}

void processTests()
{
    UNITY_BEGIN();
    RUN_TEST(test_lex_emits_expected_operator_tokens);
    RUN_TEST(test_lex_handles_single_double_and_unquoted_words);
    RUN_TEST(test_lex_emits_newline_token);
    RUN_TEST(test_lex_allows_large_token_sequences_without_exhaustion);
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

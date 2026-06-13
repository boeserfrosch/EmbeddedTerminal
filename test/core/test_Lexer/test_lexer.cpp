#include <unity.h>
#include "lang/LangAPI.h"

using namespace EmbeddedTerminal;
using namespace Lang;

void setUp(void)
{
}
void tearDown(void) {}

void test_lexer_simple_words(void)
{
    Lexer lexer;
    ETVector<token_t> tokens = lexer.tokenize("hello world");

    TEST_ASSERT_EQUAL(3, tokens.size());
    TEST_ASSERT_EQUAL_STRING("hello", tokens[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("world", tokens[1].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[2].type);
}

void test_lexer_with_unfinished_quote(void)
{
    Lexer lexer;
    ETVector<token_t> tokens = lexer.tokenize("hello \"world");
    TEST_ASSERT_EQUAL(3, tokens.size());
    TEST_ASSERT_EQUAL_STRING("hello", tokens[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("world", tokens[1].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::DOUBLE_QUOTE_STRING_LITERAL, tokens[1].type);
    TEST_ASSERT_FALSE(tokens[1].terminated);
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[2].type);
}

void test_lexer_with_escaped_quote(void)
{
    Lexer lexer;
    ETVector<token_t> tokens = lexer.tokenize("hello \\\"world\\\"");

    TEST_ASSERT_EQUAL(3, tokens.size());
    TEST_ASSERT_EQUAL_STRING("hello", tokens[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("\\\"world\\\"", tokens[1].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[1].type);
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[2].type);
}

void test_lexer_do_not_split_in_quotes(void)
{
    Lexer lexer;
    ETVector<token_t> tokens = lexer.tokenize("hello \"world is big\" today");

    TEST_ASSERT_EQUAL(4, tokens.size());
    TEST_ASSERT_EQUAL_STRING("hello", tokens[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("world is big", tokens[1].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::DOUBLE_QUOTE_STRING_LITERAL, tokens[1].type);
    TEST_ASSERT_TRUE(tokens[1].terminated);
    TEST_ASSERT_EQUAL_STRING("today", tokens[2].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[3].type);
}

void test_lexer_with_quotes_in_quotes(void)
{
    Lexer lexer;
    ETVector<token_t> tokens = lexer.tokenize("hello \"world 'is' big\" today");
    TEST_ASSERT_EQUAL(4, tokens.size());
    TEST_ASSERT_EQUAL_STRING("hello", tokens[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("world 'is' big", tokens[1].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::DOUBLE_QUOTE_STRING_LITERAL, tokens[1].type);
    TEST_ASSERT_TRUE(tokens[1].terminated);
    TEST_ASSERT_EQUAL_STRING("today", tokens[2].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[3].type);
}

void test_lexer_with_special_characters(void)
{
    Lexer lexer;
    ETVector<token_t> tokens = lexer.tokenize("echo $HOME && ls -l | grep \"my file\" \t");
    TEST_ASSERT_EQUAL(9, tokens.size());
    TEST_ASSERT_EQUAL_STRING("echo", tokens[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("HOME", tokens[1].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::VARIABLE, tokens[1].type);
    TEST_ASSERT_EQUAL_STRING("&&", tokens[2].text.c_str());
    TEST_ASSERT_EQUAL_STRING("ls", tokens[3].text.c_str());
    TEST_ASSERT_EQUAL_STRING("-l", tokens[4].text.c_str());
    TEST_ASSERT_EQUAL_STRING("|", tokens[5].text.c_str());
    TEST_ASSERT_EQUAL_STRING("grep", tokens[6].text.c_str());
    TEST_ASSERT_EQUAL_STRING("my file", tokens[7].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::DOUBLE_QUOTE_STRING_LITERAL, tokens[7].type);
    TEST_ASSERT_TRUE(tokens[7].terminated);
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[8].type);
}

void test_lexer_end_of_quote_splits(void)
{
    Lexer lexer;
    ETVector<token_t> tokens = lexer.tokenize("echo \"quote\"unmatched quote is bad");
    TEST_ASSERT_EQUAL(7, tokens.size());
    TEST_ASSERT_EQUAL_STRING("echo", tokens[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("quote", tokens[1].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::DOUBLE_QUOTE_STRING_LITERAL, tokens[1].type);
    TEST_ASSERT_TRUE(tokens[1].terminated);
    TEST_ASSERT_EQUAL_STRING("unmatched", tokens[2].text.c_str());
    TEST_ASSERT_EQUAL_STRING("quote", tokens[3].text.c_str());
    TEST_ASSERT_EQUAL_STRING("is", tokens[4].text.c_str());
    TEST_ASSERT_EQUAL_STRING("bad", tokens[5].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[6].type);
}

void test_lexer_multiple_spaces(void)
{
    Lexer lexer;
    ETVector<token_t> tokens = lexer.tokenize("   echo    hello   world   ");
    TEST_ASSERT_EQUAL(4, tokens.size());
    TEST_ASSERT_EQUAL_STRING("echo", tokens[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("hello", tokens[1].text.c_str());
    TEST_ASSERT_EQUAL_STRING("world", tokens[2].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[3].type);
}

void test_lexer_with_all_mixup_of_quotes_and_spaces(void)
{
    Lexer lexer;
    ETVector<token_t> tokens = lexer.tokenize("   echo  \"hello   world\"  'and   universe'  \"But \\\not this\"   \"''\"");
    TEST_ASSERT_EQUAL(6, tokens.size());
    TEST_ASSERT_EQUAL_STRING("echo", tokens[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("hello   world", tokens[1].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::DOUBLE_QUOTE_STRING_LITERAL, tokens[1].type);
    TEST_ASSERT_TRUE(tokens[1].terminated);
    TEST_ASSERT_EQUAL_STRING("and   universe", tokens[2].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::SINGLE_QUOTE_STRING_LITERAL, tokens[2].type);
    TEST_ASSERT_TRUE(tokens[2].terminated);
    TEST_ASSERT_EQUAL_STRING("But \\\not this", tokens[3].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::DOUBLE_QUOTE_STRING_LITERAL, tokens[3].type);
    TEST_ASSERT_TRUE(tokens[3].terminated);
    TEST_ASSERT_EQUAL_STRING("''", tokens[4].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::DOUBLE_QUOTE_STRING_LITERAL, tokens[4].type);
    TEST_ASSERT_TRUE(tokens[4].terminated);
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[5].type);
}

void test_lexer_all_operators(void)
{
    Lexer lexer;
    ETVector<token_t> tokens = lexer.tokenize("cmd1 | cmd2 > output.txt < input.txt && cmd3 || cmd4; (cmd5) >> append.txt; test \"()\"");
    TEST_ASSERT_EQUAL(21, tokens.size());
    TEST_ASSERT_EQUAL_STRING("cmd1", tokens[0].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::PIPE, tokens[1].type);
    TEST_ASSERT_EQUAL_STRING("cmd2", tokens[2].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::REDIR_OUT, tokens[3].type);
    TEST_ASSERT_EQUAL_STRING("output.txt", tokens[4].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::REDIR_IN, tokens[5].type);
    TEST_ASSERT_EQUAL_STRING("input.txt", tokens[6].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::AND_AND, tokens[7].type);
    TEST_ASSERT_EQUAL_STRING("cmd3", tokens[8].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::OR_OR, tokens[9].type);
    TEST_ASSERT_EQUAL_STRING("cmd4", tokens[10].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::SEMI, tokens[11].type);
    TEST_ASSERT_EQUAL(TokenType::PAREN_OPEN, tokens[12].type);
    TEST_ASSERT_EQUAL_STRING("cmd5", tokens[13].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::PAREN_CLOSE, tokens[14].type);
    TEST_ASSERT_EQUAL(TokenType::REDIR_APPEND, tokens[15].type);
    TEST_ASSERT_EQUAL_STRING("append.txt", tokens[16].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::SEMI, tokens[17].type);
    TEST_ASSERT_EQUAL_STRING("test", tokens[18].text.c_str());
    TEST_ASSERT_EQUAL_STRING("()", tokens[19].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::DOUBLE_QUOTE_STRING_LITERAL, tokens[19].type);
    TEST_ASSERT_TRUE(tokens[19].terminated);
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[20].type);
}

void test_lexer_all_parantheses_and_semis(void)
{
    Lexer lexer;
    ETVector<token_t> tokens = lexer.tokenize("(cmd1); [cmd2]; {cmd3}; ([{)]})");
    TEST_ASSERT_EQUAL(20, tokens.size());
    TEST_ASSERT_EQUAL(TokenType::PAREN_OPEN, tokens[0].type);
    TEST_ASSERT_EQUAL_STRING("cmd1", tokens[1].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::PAREN_CLOSE, tokens[2].type);
    TEST_ASSERT_EQUAL(TokenType::SEMI, tokens[3].type);
    TEST_ASSERT_EQUAL(TokenType::SQUARE_OPEN, tokens[4].type);
    TEST_ASSERT_EQUAL_STRING("cmd2", tokens[5].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::SQUARE_CLOSE, tokens[6].type);
    TEST_ASSERT_EQUAL(TokenType::SEMI, tokens[7].type);
    TEST_ASSERT_EQUAL(TokenType::CURLY_OPEN, tokens[8].type);
    TEST_ASSERT_EQUAL_STRING("cmd3", tokens[9].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::CURLY_CLOSE, tokens[10].type);
    TEST_ASSERT_EQUAL(TokenType::SEMI, tokens[11].type);
    TEST_ASSERT_EQUAL(TokenType::PAREN_OPEN, tokens[12].type);
    TEST_ASSERT_EQUAL(TokenType::SQUARE_OPEN, tokens[13].type);
    TEST_ASSERT_EQUAL(TokenType::CURLY_OPEN, tokens[14].type);
    TEST_ASSERT_EQUAL(TokenType::PAREN_CLOSE, tokens[15].type);
    TEST_ASSERT_EQUAL(TokenType::SQUARE_CLOSE, tokens[16].type);
    TEST_ASSERT_EQUAL(TokenType::CURLY_CLOSE, tokens[17].type);
    TEST_ASSERT_EQUAL(TokenType::PAREN_CLOSE, tokens[18].type);
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[19].type);
}

void test_lexer_with_comments(void)
{
    Lexer lexer;
    ETVector<token_t> tokens = lexer.tokenize("echo hello // this is a comment\nand this is code /* block comment */ end");
    TEST_ASSERT_EQUAL(8, tokens.size());
    TEST_ASSERT_EQUAL_STRING("echo", tokens[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("hello", tokens[1].text.c_str());
    TEST_ASSERT_EQUAL_STRING("and", tokens[2].text.c_str());
    TEST_ASSERT_EQUAL_STRING("this", tokens[3].text.c_str());
    TEST_ASSERT_EQUAL_STRING("is", tokens[4].text.c_str());
    TEST_ASSERT_EQUAL_STRING("code", tokens[5].text.c_str());
    TEST_ASSERT_EQUAL_STRING("end", tokens[6].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[7].type);
}

void test_lexer_with_all_sorts_of_whitespaces(void)
{
    Lexer lexer;
    ETVector<token_t> tokens = lexer.tokenize("echo\t\t\"hello world\"\nthis is a test\r\n");
    TEST_ASSERT_EQUAL(9, tokens.size());
    TEST_ASSERT_EQUAL_STRING("echo", tokens[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("hello world", tokens[1].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::DOUBLE_QUOTE_STRING_LITERAL, tokens[1].type);
    TEST_ASSERT_EQUAL(TokenType::NEWLINE, tokens[2].type);
    TEST_ASSERT_EQUAL_STRING("this", tokens[3].text.c_str());
    TEST_ASSERT_EQUAL_STRING("is", tokens[4].text.c_str());
    TEST_ASSERT_EQUAL_STRING("a", tokens[5].text.c_str());
    TEST_ASSERT_EQUAL_STRING("test", tokens[6].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::NEWLINE, tokens[7].type);
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[8].type);
}

void test_lexer_variable_parsing(void)
{
    Lexer lexer;
    ETVector<token_t> tokens = lexer.tokenize("echo $HOME ${USER} $UNFINISHED_VAR");
    // We do not support complex variable parsing with braces and such, so we expect to just get the variable name without the $ or braces, and treat the braces as separate tokens. The unfinished variable should be parsed as a variable token with the text being everything after the $ since we don't find any valid variable characters after it. But we set it as unfinished (terminated = false) since it likely indicates a syntax error that the caller might want to handle.
    TEST_ASSERT_EQUAL(8, tokens.size());
    TEST_ASSERT_EQUAL_STRING("echo", tokens[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("HOME", tokens[1].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::VARIABLE, tokens[1].type);
    TEST_ASSERT_EQUAL_STRING("", tokens[2].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::VARIABLE, tokens[2].type);
    TEST_ASSERT_TRUE(tokens[2].terminated);
    TEST_ASSERT_EQUAL(TokenType::CURLY_OPEN, tokens[3].type);
    TEST_ASSERT_EQUAL_STRING("USER", tokens[4].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::CURLY_CLOSE, tokens[5].type);
    TEST_ASSERT_EQUAL_STRING("UNFINISHED_VAR", tokens[6].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::VARIABLE, tokens[6].type);
    TEST_ASSERT_TRUE(tokens[6].terminated);
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[7].type);
}

void test_lexer_find_reservered_words(void)
{
    Lexer lexer;
    ETVector<token_t> tokens = lexer.tokenize("if then else fi for while do done true false");
    TEST_ASSERT_EQUAL(11, tokens.size());
    TEST_ASSERT_EQUAL(TokenType::IF, tokens[0].type);
    TEST_ASSERT_EQUAL(TokenType::THEN, tokens[1].type);
    TEST_ASSERT_EQUAL(TokenType::ELSE, tokens[2].type);
    TEST_ASSERT_EQUAL(TokenType::FI, tokens[3].type);
    TEST_ASSERT_EQUAL(TokenType::FOR, tokens[4].type);
    TEST_ASSERT_EQUAL(TokenType::WHILE, tokens[5].type);
    TEST_ASSERT_EQUAL(TokenType::DO, tokens[6].type);
    TEST_ASSERT_EQUAL(TokenType::DONE, tokens[7].type);
    TEST_ASSERT_EQUAL(TokenType::TRUE, tokens[8].type);
    TEST_ASSERT_EQUAL(TokenType::FALSE, tokens[9].type);
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[10].type);
}

void test_lexer_do_not_find_reserved_words_in_quotes(void)
{
    Lexer lexer;
    ETVector<token_t> tokens = lexer.tokenize("echo \"if then else\" 'for while' do");
    TEST_ASSERT_EQUAL(5, tokens.size());
    TEST_ASSERT_EQUAL_STRING("echo", tokens[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("if then else", tokens[1].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::DOUBLE_QUOTE_STRING_LITERAL, tokens[1].type);
    TEST_ASSERT_TRUE(tokens[1].terminated);
    TEST_ASSERT_EQUAL_STRING("for while", tokens[2].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::SINGLE_QUOTE_STRING_LITERAL, tokens[2].type);
    TEST_ASSERT_TRUE(tokens[2].terminated);
    TEST_ASSERT_EQUAL(TokenType::DO, tokens[3].type);
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[4].type);
}

void test_lexer_do_not_reserved_words_in_long_words(void)
{
    Lexer lexer;
    ETVector<token_t> tokens = lexer.tokenize("echo iffy formless");
    TEST_ASSERT_EQUAL(4, tokens.size());
    TEST_ASSERT_EQUAL_STRING("echo", tokens[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("iffy", tokens[1].text.c_str());
    TEST_ASSERT_EQUAL_STRING("formless", tokens[2].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[1].type);
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[2].type);
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[3].type);
}

void test_lexer_simple_for_loop(void)
{
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("for i = 1 2 3 do echo $i done");
    TEST_ASSERT_EQUAL(11, tokens.size());
    TEST_ASSERT_EQUAL(TokenType::FOR, tokens[0].type);
    TEST_ASSERT_EQUAL_STRING("i", tokens[1].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[1].type);
    TEST_ASSERT_EQUAL(TokenType::EQUALS, tokens[2].type);
    TEST_ASSERT_EQUAL_STRING("1", tokens[3].text.c_str());
    TEST_ASSERT_EQUAL_STRING("2", tokens[4].text.c_str());
    TEST_ASSERT_EQUAL_STRING("3", tokens[5].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[3].type);
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[4].type);
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[5].type);
    TEST_ASSERT_EQUAL(TokenType::DO, tokens[6].type);
    TEST_ASSERT_EQUAL_STRING("echo", tokens[7].text.c_str());
    TEST_ASSERT_EQUAL_STRING("i", tokens[8].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::VARIABLE, tokens[8].type);
    TEST_ASSERT_EQUAL(TokenType::DONE, tokens[9].type);
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[10].type);
}

void test_lexer_comparison_operators(void)
{
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("if [ \"$VAR\" == \"value\" ] && [ \"$VAR\" != \"other\" ]; then echo true; else echo false; fi");
    TEST_ASSERT_EQUAL(23, tokens.size());
    TEST_ASSERT_EQUAL(TokenType::IF, tokens[0].type);
    TEST_ASSERT_EQUAL(TokenType::SQUARE_OPEN, tokens[1].type);
    TEST_ASSERT_EQUAL_STRING("$VAR", tokens[2].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::DOUBLE_QUOTE_STRING_LITERAL, tokens[2].type);
    TEST_ASSERT_TRUE(tokens[2].terminated);
    TEST_ASSERT_EQUAL_STRING("==", tokens[3].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::EQUALS_EQUALS, tokens[3].type);
    TEST_ASSERT_EQUAL_STRING("value", tokens[4].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::DOUBLE_QUOTE_STRING_LITERAL, tokens[4].type);
    TEST_ASSERT_TRUE(tokens[4].terminated);
    TEST_ASSERT_EQUAL(TokenType::SQUARE_CLOSE, tokens[5].type);
    TEST_ASSERT_EQUAL(TokenType::AND_AND, tokens[6].type);
    TEST_ASSERT_EQUAL(TokenType::SQUARE_OPEN, tokens[7].type);
    TEST_ASSERT_EQUAL_STRING("$VAR", tokens[8].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::DOUBLE_QUOTE_STRING_LITERAL, tokens[8].type);
    TEST_ASSERT_TRUE(tokens[8].terminated);
    TEST_ASSERT_EQUAL_STRING("!=", tokens[9].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::NOT_EQUALS, tokens[9].type);
    TEST_ASSERT_EQUAL_STRING("other", tokens[10].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::DOUBLE_QUOTE_STRING_LITERAL, tokens[10].type);
    TEST_ASSERT_TRUE(tokens[10].terminated);
    TEST_ASSERT_EQUAL(TokenType::SQUARE_CLOSE, tokens[11].type);
    TEST_ASSERT_EQUAL(TokenType::SEMI, tokens[12].type);
    TEST_ASSERT_EQUAL(TokenType::THEN, tokens[13].type);
    TEST_ASSERT_EQUAL_STRING("echo", tokens[14].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::TRUE, tokens[15].type);
    TEST_ASSERT_EQUAL(TokenType::SEMI, tokens[16].type);
    TEST_ASSERT_EQUAL(TokenType::ELSE, tokens[17].type);
    TEST_ASSERT_EQUAL_STRING("echo", tokens[18].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::FALSE, tokens[19].type);
    TEST_ASSERT_EQUAL(TokenType::SEMI, tokens[20].type);
    TEST_ASSERT_EQUAL(TokenType::FI, tokens[21].type);
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[22].type);
    // We won't check the rest of the tokens in detail here since we already verified that reserved words are correctly identified and that operators are correctly identified. We just want to make sure the comparison operators are correctly tokenized and not split into separate = or ! and = tokens.
}

void test_lexer_not_equals_operator(void)
{
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("!= ! = ");
    TEST_ASSERT_EQUAL(4, tokens.size());
    TEST_ASSERT_EQUAL(TokenType::NOT_EQUALS, tokens[0].type);
    TEST_ASSERT_EQUAL(TokenType::NOT, tokens[1].type);
    TEST_ASSERT_EQUAL(TokenType::EQUALS, tokens[2].type);
}

void test_lexer_function_definition(void)
{
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("function myFunc[i j] echo myFunc\n echo $i; echo $j done");
    TEST_ASSERT_EQUAL(16, tokens.size());
    TEST_ASSERT_EQUAL(TokenType::FUNCTION, tokens[0].type);
    TEST_ASSERT_EQUAL_STRING("myFunc", tokens[1].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[1].type);
    TEST_ASSERT_EQUAL(TokenType::SQUARE_OPEN, tokens[2].type);
    TEST_ASSERT_EQUAL_STRING("i", tokens[3].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[3].type);
    TEST_ASSERT_EQUAL_STRING("j", tokens[4].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[4].type);
    TEST_ASSERT_EQUAL(TokenType::SQUARE_CLOSE, tokens[5].type);
    TEST_ASSERT_EQUAL_STRING("echo", tokens[6].text.c_str());
    TEST_ASSERT_EQUAL_STRING("myFunc", tokens[7].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[7].type);
    TEST_ASSERT_EQUAL(TokenType::NEWLINE, tokens[8].type);
    TEST_ASSERT_EQUAL_STRING("echo", tokens[9].text.c_str());
    TEST_ASSERT_EQUAL_STRING("i", tokens[10].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::VARIABLE, tokens[10].type);
    TEST_ASSERT_EQUAL(TokenType::SEMI, tokens[11].type);
    TEST_ASSERT_EQUAL_STRING("echo", tokens[12].text.c_str());
    TEST_ASSERT_EQUAL_STRING("j", tokens[13].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::VARIABLE, tokens[13].type);
    TEST_ASSERT_EQUAL(TokenType::DONE, tokens[14].type);
    TEST_ASSERT_EQUAL(TokenType::END_OF_FILE, tokens[15].type);
}

void test_lexer_variable_assignment_without_whitespace(void)
{
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("VAR=value echo $VAR");
    TEST_ASSERT_EQUAL(6, tokens.size());
    TEST_ASSERT_EQUAL_STRING("VAR", tokens[0].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[0].type);
    TEST_ASSERT_EQUAL(TokenType::EQUALS, tokens[1].type);
    TEST_ASSERT_EQUAL_STRING("value", tokens[2].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[2].type);
    TEST_ASSERT_EQUAL_STRING("echo", tokens[3].text.c_str());
    TEST_ASSERT_EQUAL_STRING("VAR", tokens[4].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::VARIABLE, tokens[4].type);
}

void test_lexer_tokenize_path_literals(void)
{
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("echo /path/to/file /another/path /");
    TEST_ASSERT_EQUAL(5, tokens.size());
    TEST_ASSERT_EQUAL_STRING("echo", tokens[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("/path/to/file", tokens[1].text.c_str());
    TEST_ASSERT_EQUAL_STRING("/another/path", tokens[2].text.c_str());
    TEST_ASSERT_EQUAL_STRING("/", tokens[3].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[1].type);
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[2].type);
    TEST_ASSERT_EQUAL(TokenType::WORD, tokens[3].type);
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_lexer_simple_words);
    RUN_TEST(test_lexer_with_unfinished_quote);
    RUN_TEST(test_lexer_with_escaped_quote);
    RUN_TEST(test_lexer_do_not_split_in_quotes);
    RUN_TEST(test_lexer_with_quotes_in_quotes);
    RUN_TEST(test_lexer_with_special_characters);
    RUN_TEST(test_lexer_end_of_quote_splits);
    RUN_TEST(test_lexer_multiple_spaces);
    RUN_TEST(test_lexer_with_all_mixup_of_quotes_and_spaces);
    RUN_TEST(test_lexer_all_operators);
    RUN_TEST(test_lexer_all_parantheses_and_semis);
    RUN_TEST(test_lexer_with_comments);
    RUN_TEST(test_lexer_with_all_sorts_of_whitespaces);
    RUN_TEST(test_lexer_variable_parsing);
    RUN_TEST(test_lexer_find_reservered_words);
    RUN_TEST(test_lexer_do_not_find_reserved_words_in_quotes);
    RUN_TEST(test_lexer_do_not_reserved_words_in_long_words);
    RUN_TEST(test_lexer_simple_for_loop);
    RUN_TEST(test_lexer_comparison_operators);
    RUN_TEST(test_lexer_not_equals_operator);
    RUN_TEST(test_lexer_function_definition);
    RUN_TEST(test_lexer_variable_assignment_without_whitespace);
    RUN_TEST(test_lexer_tokenize_path_literals);
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
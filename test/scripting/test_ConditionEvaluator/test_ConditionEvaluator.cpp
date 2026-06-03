#include <unity.h>

#include "Terminal.h"
#include "../../Mocks/MockTerminal.h"
#include "../../Mocks/MockCommand.h"
#include "scripting/Runner.h"

using namespace EmbeddedTerminal;
using namespace EmbeddedTerminal::Scripting;

Expression trueLiteral = Expression::createLiteral({"true", TokenType::TRUE});
Expression falseLiteral = Expression::createLiteral({"false", TokenType::FALSE});
Expression zeroLiteral = Expression::createLiteral({"0", TokenType::WORD});
Expression nonEmptyStringLiteral = Expression::createLiteral({"\"hello\"", TokenType::DOUBLE_QUOTE_STRING_LITERAL});
Expression wordLiteral = Expression::createLiteral({"word", TokenType::WORD});

void setUp(void) {}
void tearDown(void) {}

class RunnerHarness : public Runner
{
public:
    RunnerHarness(IExecutionContext &ctx) : Runner(ctx) {}

    bool evaluateCondition(const ConditionExpression &condition, ETMap<ETString, ETString> &variables, bool &result)
    {
        return Runner::evaluateCondition(condition, variables, result);
    }
};

class ScriptParserHarness : public ScriptParser
{
public:
    Expression buildConditionExpression(const token_list_t &tokens, size_t &index, size_t endIndex)
    {
        return ScriptParser::buildConditionExpression(tokens, index, endIndex);
    }

    void resetError()
    {
        error_ = ScriptParser::ErrorCode::None;
    }
};

void test_condition_parser_true_literal(void)
{
    MockTerminal terminal;
    RunnerHarness runner(terminal);
    ETMap<ETString, ETString> variables;
    auto expr = Expression::createConditionExpression({{trueLiteral, false}}, {});
    bool result = false;
    TEST_ASSERT_TRUE(runner.evaluateCondition(expr.data.condition, variables, result));
    TEST_ASSERT_TRUE(result);
}

void test_condition_parser_false_literal(void)
{
    MockTerminal terminal;
    RunnerHarness runner(terminal);
    ETMap<ETString, ETString> variables;
    auto expr = Expression::createConditionExpression({{falseLiteral, false}}, {});
    bool result = false;
    bool ret = runner.evaluateCondition(expr.data.condition, variables, result);
    TEST_ASSERT_FALSE(runner.error());
    TEST_ASSERT_TRUE(ret);
    TEST_ASSERT_FALSE(result);
}

void test_condition_parser_zero_literal(void)
{
    MockTerminal terminal;
    RunnerHarness runner(terminal);
    ETMap<ETString, ETString> variables;
    auto expr = Expression::createConditionExpression({{zeroLiteral, false}}, {});
    bool result = false;
    TEST_ASSERT_TRUE(runner.evaluateCondition(expr.data.condition, variables, result));
    TEST_ASSERT_FALSE(result);
}

void test_condition_parser_nonempty_string_literal(void)
{
    MockTerminal terminal;
    RunnerHarness runner(terminal);
    ETMap<ETString, ETString> variables;
    auto expr = Expression::createConditionExpression({{nonEmptyStringLiteral, false}}, {});
    bool result = false;
    TEST_ASSERT_TRUE(runner.evaluateCondition(expr.data.condition, variables, result));
    TEST_ASSERT_TRUE(result);
}

void test_condition_parser_or_operator(void)
{
    MockTerminal terminal;
    RunnerHarness runner(terminal);
    ETMap<ETString, ETString> variables;
    token_list_t tokens;
    auto conditionNodes = ETVector<ETPair<Expression, bool>>{{trueLiteral, false}, {falseLiteral, false}};
    auto connectingOperators = ETVector<TokenType>{TokenType::OR_OR};
    auto expr = Expression::createConditionExpression(conditionNodes, connectingOperators);
    bool result = false;
    TEST_ASSERT_TRUE(runner.evaluateCondition(expr.data.condition, variables, result));
    TEST_ASSERT_TRUE(result);
}

void test_condition_parser_and_operator(void)
{
    MockTerminal terminal;
    RunnerHarness runner(terminal);
    ETMap<ETString, ETString> variables;
    auto conditionNodes = ETVector<ETPair<Expression, bool>>{{trueLiteral, false}, {falseLiteral, false}};
    auto connectingOperators = ETVector<TokenType>{TokenType::AND_AND};
    auto expr = Expression::createConditionExpression(conditionNodes, connectingOperators);
    bool result = false;
    TEST_ASSERT_TRUE(runner.evaluateCondition(expr.data.condition, variables, result));
    TEST_ASSERT_FALSE(result);
}

void test_condition_parser_parentheses(void)
{
    MockTerminal terminal;
    RunnerHarness runner(terminal);
    ETMap<ETString, ETString> variables;
    token_list_t tokens;
    tokens.push_back({"(", TokenType::PAREN_OPEN});
    tokens.push_back({"true", TokenType::TRUE});
    tokens.push_back({"||", TokenType::OR_OR});
    tokens.push_back({"false", TokenType::FALSE});
    tokens.push_back({")", TokenType::PAREN_CLOSE});
    tokens.push_back({"&&", TokenType::AND_AND});
    tokens.push_back({"false", TokenType::FALSE});

    auto innerConditionNodes = ETVector<ETPair<Expression, bool>>{{trueLiteral, false}, {falseLiteral, false}};
    auto innerConnectingOperators = ETVector<TokenType>{TokenType::OR_OR};
    auto conditionNodes = ETVector<ETPair<Expression, bool>>{{Expression::createConditionExpression(innerConditionNodes, innerConnectingOperators), false}, {falseLiteral, false}};
    auto connectingOperators = ETVector<TokenType>{TokenType::AND_AND};
    auto expr = Expression::createConditionExpression(conditionNodes, connectingOperators);

    bool result = false;
    TEST_ASSERT_TRUE(runner.evaluateCondition(expr.data.condition, variables, result));
    TEST_ASSERT_FALSE(result);
}

void test_condition_parser_unexpected_token(void)
{
    MockTerminal terminal;
    RunnerHarness runner(terminal);
    ETMap<ETString, ETString> variables;

    auto conditionNodes = ETVector<ETPair<Expression, bool>>{{trueLiteral, false}, {wordLiteral, false}};
    auto connectingOperators = ETVector<TokenType>{};
    auto expr = Expression::createConditionExpression(conditionNodes, connectingOperators);

    bool result = false;
    TEST_ASSERT_FALSE(runner.evaluateCondition(expr.data.condition, variables, result));
    TEST_ASSERT_TRUE(runner.error());
    TEST_ASSERT_EQUAL(Runner::ErrorCode::SyntaxError, runner.getLastError());
}

void test_condition_parser_missing_operand(void)
{
    MockTerminal terminal;
    RunnerHarness runner(terminal);
    ETMap<ETString, ETString> variables;

    auto conditionNodes = ETVector<ETPair<Expression, bool>>{{trueLiteral, false}};
    auto connectingOperators = ETVector<TokenType>{TokenType::AND_AND};
    auto expr = Expression::createConditionExpression(conditionNodes, connectingOperators);

    bool result = false;
    TEST_ASSERT_FALSE(runner.evaluateCondition(expr.data.condition, variables, result));
    TEST_ASSERT_TRUE(runner.error());
    TEST_ASSERT_EQUAL(Runner::ErrorCode::SyntaxError, runner.getLastError());
}

void test_condition_parser_comparison_of_string(void)
{
    MockTerminal terminal;
    RunnerHarness runner(terminal);
    CapturingCommand echoCommand("ok", 0);
    terminal.registerCommand("echo", &echoCommand);
    ETMap<ETString, ETString> variables;

    token_list_t tokens = Lexer().tokenize("[ \"ok\" == \"ok\" ]");
    ScriptParserHarness parser;
    size_t index = 0;
    auto conditionExpr = parser.buildConditionExpression(tokens, index, tokens.size());

    if (parser.error())
    {
        TEST_MESSAGE(("Parser error: " + toETString(static_cast<int>(parser.getError()))).c_str());
        TEST_MESSAGE(("Parsing stopped at token index: " + toETString(index)).c_str());
    }
    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL(ExpressionType::Condition, conditionExpr.tag);
    bool result = false;
    TEST_ASSERT_TRUE(runner.evaluateCondition(conditionExpr.data.condition, variables, result));

    TEST_ASSERT_FALSE(runner.error());
    TEST_ASSERT_TRUE(result);
}

void test_condition_parser_string_comparison(void)
{
    MockTerminal terminal;
    RunnerHarness runner(terminal);
    ETMap<ETString, ETString> variables;

    token_list_t tokens;
    tokens.push_back({"[", TokenType::SQUARE_OPEN});
    tokens.push_back({"bla", TokenType::DOUBLE_QUOTE_STRING_LITERAL});
    tokens.push_back({"==", TokenType::EQUALS_EQUALS});
    tokens.push_back({"bla", TokenType::SINGLE_QUOTE_STRING_LITERAL});
    tokens.push_back({"]", TokenType::SQUARE_CLOSE});

    ScriptParserHarness parser;
    size_t index = 0;
    auto conditionExpr = parser.buildConditionExpression(tokens, index, tokens.size());

    TEST_ASSERT_TRUE(conditionExpr.tag == ExpressionType::Condition);
    bool result = false;
    TEST_ASSERT_TRUE(runner.evaluateCondition(conditionExpr.data.condition, variables, result));

    TEST_ASSERT_FALSE(runner.error());
    TEST_ASSERT_TRUE(result);
}

void test_condition_parser_command_block_syntax_errors(void)
{
    MockTerminal terminal;
    RunnerHarness runner(terminal);
    ETMap<ETString, ETString> variables;

    token_list_t missingClose;
    missingClose.push_back({"{", TokenType::CURLY_OPEN});
    missingClose.push_back({"echo", TokenType::WORD});

    ScriptParserHarness parser;
    size_t index = 0;
    auto conditionExpr = parser.buildConditionExpression(missingClose, index, missingClose.size());

    TEST_ASSERT_FALSE(conditionExpr.tag == ExpressionType::Condition);

    token_list_t emptyBlock;
    emptyBlock.push_back({"{", TokenType::CURLY_OPEN});
    emptyBlock.push_back({"}", TokenType::CURLY_CLOSE});

    index = 0;
    parser.resetError();
    auto conditionExpr2 = parser.buildConditionExpression(emptyBlock, index, emptyBlock.size());

    TEST_ASSERT_FALSE(conditionExpr2.tag == ExpressionType::Condition);
}

void test_complex_condition(void)
{
    MockTerminal terminal;
    RunnerHarness runner(terminal);
    ETMap<ETString, ETString> variables;
    variables["foo"] = "ok";
    token_list_t tokens;
    tokens.push_back({"false", TokenType::FALSE});
    tokens.push_back({"||", TokenType::OR_OR});
    tokens.push_back({"(", TokenType::PAREN_OPEN});
    tokens.push_back({"false", TokenType::FALSE});
    tokens.push_back({"||", TokenType::OR_OR});
    tokens.push_back({"(", TokenType::PAREN_OPEN});
    tokens.push_back({"[", TokenType::SQUARE_OPEN});
    tokens.push_back({"foo", TokenType::VARIABLE});
    tokens.push_back({"==", TokenType::EQUALS_EQUALS});
    tokens.push_back({"ok", TokenType::DOUBLE_QUOTE_STRING_LITERAL});
    tokens.push_back({"]", TokenType::SQUARE_CLOSE});
    tokens.push_back({")", TokenType::PAREN_CLOSE});
    tokens.push_back({"&&", TokenType::AND_AND});
    tokens.push_back({"true", TokenType::TRUE});
    tokens.push_back({")", TokenType::PAREN_CLOSE});
    tokens.push_back({"||", TokenType::OR_OR});
    tokens.push_back({"[", TokenType::SQUARE_OPEN});
    tokens.push_back({"foo", TokenType::VARIABLE});
    tokens.push_back({"!=", TokenType::NOT_EQUALS});
    tokens.push_back({"err", TokenType::DOUBLE_QUOTE_STRING_LITERAL});
    tokens.push_back({"]", TokenType::SQUARE_CLOSE});

    ScriptParserHarness parser;
    size_t index = 0;
    auto conditionExpr = parser.buildConditionExpression(tokens, index, tokens.size());

    TEST_ASSERT_FALSE(parser.error());

    TEST_ASSERT_TRUE(conditionExpr.tag == ExpressionType::Condition);
    bool result = false;
    TEST_ASSERT_TRUE(runner.evaluateCondition(conditionExpr.data.condition, variables, result));

    TEST_ASSERT_FALSE(runner.error());
    TEST_ASSERT_TRUE(result);
}

void test_complex_condition_evaluating_to_false(void)
{
    MockTerminal terminal;
    RunnerHarness runner(terminal);
    ETMap<ETString, ETString> variables;
    variables["foo"] = "ok";
    token_list_t tokens;
    tokens.push_back({"false", TokenType::FALSE});
    tokens.push_back({"||", TokenType::OR_OR});
    tokens.push_back({"(", TokenType::PAREN_OPEN});
    tokens.push_back({"false", TokenType::FALSE});
    tokens.push_back({"||", TokenType::OR_OR});
    tokens.push_back({"(", TokenType::PAREN_OPEN});
    tokens.push_back({"[", TokenType::SQUARE_OPEN});
    tokens.push_back({"foo", TokenType::VARIABLE});
    tokens.push_back({"==", TokenType::EQUALS_EQUALS});
    tokens.push_back({"ok", TokenType::DOUBLE_QUOTE_STRING_LITERAL});
    tokens.push_back({"]", TokenType::SQUARE_CLOSE});
    tokens.push_back({")", TokenType::PAREN_CLOSE});
    tokens.push_back({"&&", TokenType::AND_AND});
    tokens.push_back({"true", TokenType::TRUE});
    tokens.push_back({")", TokenType::PAREN_CLOSE});
    tokens.push_back({"&&", TokenType::AND_AND});
    tokens.push_back({"[", TokenType::SQUARE_OPEN});
    tokens.push_back({"foo", TokenType::VARIABLE});
    tokens.push_back({"==", TokenType::EQUALS_EQUALS});
    tokens.push_back({"err", TokenType::DOUBLE_QUOTE_STRING_LITERAL});
    tokens.push_back({"]", TokenType::SQUARE_CLOSE});

    ScriptParserHarness parser;
    size_t index = 0;
    auto conditionExpr = parser.buildConditionExpression(tokens, index, tokens.size());

    TEST_ASSERT_FALSE(parser.error());

    TEST_ASSERT_TRUE(conditionExpr.tag == ExpressionType::Condition);
    bool result = false;
    TEST_ASSERT_TRUE(runner.evaluateCondition(conditionExpr.data.condition, variables, result));

    TEST_ASSERT_FALSE(runner.error());
    TEST_ASSERT_FALSE(result);
}

void test_parser_raises_syntax_error_for_unexpected_token_in_condition(void)
{
    MockTerminal terminal;
    RunnerHarness runner(terminal);
    ETMap<ETString, ETString> variables;

    ScriptParserHarness parser;
    size_t index = 0;
    // Test unexpected string literal token in condition expression
    auto conditionExpr = parser.buildConditionExpression(Lexer().tokenize("\"true\" \"unexpected\""), index, 3);

    TEST_ASSERT_TRUE(parser.error());
    TEST_ASSERT_EQUAL(ScriptParser::ErrorCode::SyntaxError, parser.getError());
    TEST_ASSERT_EQUAL(1, index); // The syntax error should be reported at the position of the unexpected token

    // Test unexpected word token in condition expression
    index = 0; // Reset index to 0 before parsing the next condition expression
    auto conditionExpr2 = parser.buildConditionExpression(Lexer().tokenize("true && unexpected"), index, 4);
    TEST_ASSERT_TRUE(parser.error());
    TEST_ASSERT_EQUAL(ScriptParser::ErrorCode::SyntaxError, parser.getError());
    TEST_ASSERT_EQUAL(2, index); // The syntax error should be reported at the position of the unexpected token, true and && are valid tokens, but the word "unexpected" is not a valid token in a condition expression, so the syntax error should be reported at the position of the "unexpected" token

    index = 0; // Reset index to 0 before parsing the next condition expression
    auto conditionExpr3 = parser.buildConditionExpression(Lexer().tokenize("true || && false"), index, 5);
    TEST_ASSERT_TRUE(parser.error());
    TEST_ASSERT_EQUAL(ScriptParser::ErrorCode::SyntaxError, parser.getError());
    TEST_ASSERT_EQUAL(2, index); // The syntax error should be reported at the position

    // Command blocks are not allowed in condition expressions, so this should also result in a syntax error
    index = 0; // Reset index to 0 before parsing the next condition expression
    auto conditionExpr4 = parser.buildConditionExpression(Lexer().tokenize("{cmd arg0 arg1}"), index, 6);
    TEST_ASSERT_TRUE(parser.error());
    TEST_ASSERT_EQUAL(ScriptParser::ErrorCode::SyntaxError, parser.getError());
    TEST_ASSERT_EQUAL(0, index); // The syntax error should be reported at the position of the unexpected token
}

void test_condition_variables(void)
{
    MockTerminal terminal;
    RunnerHarness runner(terminal);
    ETMap<ETString, ETString> variables;
    variables["var"] = "ok";

    token_list_t tokens;
    tokens.push_back({"[", TokenType::SQUARE_OPEN});
    tokens.push_back({"var", TokenType::VARIABLE});
    tokens.push_back({"==", TokenType::EQUALS_EQUALS});
    tokens.push_back({"ok", TokenType::DOUBLE_QUOTE_STRING_LITERAL});
    tokens.push_back({"]", TokenType::SQUARE_CLOSE});

    ScriptParserHarness parser;
    size_t index = 0;
    auto conditionExpr = parser.buildConditionExpression(tokens, index, tokens.size());

    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_TRUE(conditionExpr.tag == ExpressionType::Condition);

    bool result = false;
    TEST_ASSERT_TRUE(runner.evaluateCondition(conditionExpr.data.condition, variables, result));

    TEST_ASSERT_FALSE(runner.error());
    TEST_ASSERT_TRUE(result);
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_condition_parser_true_literal);
    RUN_TEST(test_condition_parser_false_literal);
    RUN_TEST(test_condition_parser_zero_literal);
    RUN_TEST(test_condition_parser_nonempty_string_literal);
    RUN_TEST(test_condition_parser_and_operator);
    RUN_TEST(test_condition_parser_or_operator);
    RUN_TEST(test_condition_parser_parentheses);
    RUN_TEST(test_condition_parser_unexpected_token);
    RUN_TEST(test_condition_parser_missing_operand);
    RUN_TEST(test_condition_parser_comparison_of_string);
    RUN_TEST(test_parser_raises_syntax_error_for_unexpected_token_in_condition);
    RUN_TEST(test_condition_parser_string_comparison);
    RUN_TEST(test_condition_parser_command_block_syntax_errors);
    RUN_TEST(test_complex_condition);
    RUN_TEST(test_complex_condition_evaluating_to_false);
    RUN_TEST(test_condition_variables);
    UNITY_END();
}

#if (defined(ESP_PLATFORM) || defined(ESP32)) && !defined(ARDUINO)
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

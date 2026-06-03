#if (defined(ESP_PLATFORM) || defined(ESP32)) && !defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#elif defined(ARDUINO)
#include <Arduino.h>
#endif

#include <unity.h>
#include "scripting/Parser.h"

using namespace EmbeddedTerminal;
using namespace EmbeddedTerminal::Scripting;

class ScriptParserHarness : public ScriptParser
{
public:
    Expression buildPipelineExpression(const token_list_t &tokens, size_t &index, const std::vector<TokenType> &stopAt)
    {
        return ScriptParser::buildPipelineExpression(tokens, index, stopAt);
    }
};

void setUp(void)
{
}
void tearDown(void) {}

static void assert_pipeline_expression(const Expression &expression, const char *keyword, size_t argumentCount)
{
    TEST_ASSERT_EQUAL((int)ExpressionType::Pipeline, (int)expression.tag);
    TEST_ASSERT_EQUAL(ExpressionType::Command, expression.data.pipeline.commands[0].tag);
    TEST_ASSERT_EQUAL_STRING(keyword, expression.data.pipeline.commands[0].data.command.keyword.text.c_str());
    TEST_ASSERT_EQUAL_UINT(argumentCount, expression.data.pipeline.commands[0].data.command.arguments.size());
}

void test_parser_simple_command(void)
{
    ScriptParser parser;

    token_list_t tokens;
    tokens.push_back({"echo", TokenType::WORD});
    tokens.push_back({"hello world", TokenType::WORD});
    tokens.push_back({"", TokenType::END_OF_FILE});

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL_UINT(1, chain.size());
    TEST_ASSERT_EQUAL(ExpressionType::Pipeline, chain[0].tag);
    TEST_ASSERT_EQUAL(1, chain[0].data.pipeline.commands.size());
    TEST_ASSERT_EQUAL(ExpressionType::Command, chain[0].data.pipeline.commands[0].tag);
    TEST_ASSERT_EQUAL_STRING("echo", chain[0].data.pipeline.commands[0].data.command.keyword.text.c_str());
    TEST_ASSERT_EQUAL_UINT(1, chain[0].data.pipeline.commands[0].data.command.arguments.size());
    TEST_ASSERT_EQUAL_STRING("hello world", chain[0].data.pipeline.commands[0].data.command.arguments[0].text.c_str());
}

void test_parser_while_loop(void)
{
    ScriptParser parser;

    token_list_t tokens;
    tokens.push_back({"while", TokenType::WHILE});
    tokens.push_back({"count", TokenType::VARIABLE});
    tokens.push_back({"do", TokenType::DO});
    tokens.push_back({"echo", TokenType::WORD});
    tokens.push_back({"looping", TokenType::WORD});
    tokens.push_back({"done", TokenType::DONE});
    tokens.push_back({"", TokenType::END_OF_FILE});

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL_UINT(1, chain.size());
    TEST_ASSERT_EQUAL((int)ExpressionType::WhileLoop, (int)chain[0].tag);
    TEST_ASSERT_EQUAL(ExpressionType::Condition, chain[0].data.whileLoop.condition->tag);
    TEST_ASSERT_EQUAL(ExpressionType::Literal, chain[0].data.whileLoop.condition->data.condition.conditionNodes[0].first.tag);
    TEST_ASSERT_EQUAL_STRING("count", chain[0].data.whileLoop.condition->data.condition.conditionNodes[0].first.data.literal.literalToken.text.c_str());
    TEST_ASSERT_EQUAL_UINT(1, chain[0].data.whileLoop.body.size());
    TEST_ASSERT_EQUAL((int)ExpressionType::Pipeline, (int)chain[0].data.whileLoop.body[0].tag);

    // assert_pipeline_expression(chain[0].data.whileLoop.body[0], "echo", 1);
    // TEST_ASSERT_EQUAL_STRING("looping", chain[0].data.whileLoop.body[0].data.pipeline.commands[0].data.command.arguments[0].text.c_str());
}

void test_parser_for_loop_with_script_command(void)
{
    ScriptParser parser;

    token_list_t tokens;
    tokens.push_back({"for", TokenType::FOR});
    tokens.push_back({"i", TokenType::WORD});
    tokens.push_back({"=", TokenType::EQUALS});
    tokens.push_back({"1", TokenType::WORD});
    tokens.push_back({"2", TokenType::WORD});
    tokens.push_back({"3", TokenType::WORD});
    tokens.push_back({"do", TokenType::DO});
    tokens.push_back({"script", TokenType::WORD});
    tokens.push_back({"./echo.et", TokenType::WORD});
    tokens.push_back({"i", TokenType::WORD});
    tokens.push_back({"done", TokenType::DONE});
    tokens.push_back({"", TokenType::END_OF_FILE});

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL_UINT(1, chain.size());
    TEST_ASSERT_EQUAL((int)ExpressionType::ForLoop, (int)chain[0].tag);
    TEST_ASSERT_EQUAL_STRING("i", chain[0].data.forLoop.variable.c_str());
    TEST_ASSERT_EQUAL_UINT(3, chain[0].data.forLoop.values.size());
    TEST_ASSERT_EQUAL_STRING("1", chain[0].data.forLoop.values[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("2", chain[0].data.forLoop.values[1].text.c_str());
    TEST_ASSERT_EQUAL_STRING("3", chain[0].data.forLoop.values[2].text.c_str());
    TEST_ASSERT_EQUAL_UINT(1, chain[0].data.forLoop.body.size());
    assert_pipeline_expression(chain[0].data.forLoop.body[0], "script", 2);
    TEST_ASSERT_EQUAL_STRING("./echo.et", chain[0].data.forLoop.body[0].data.pipeline.commands[0].data.command.arguments[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("i", chain[0].data.forLoop.body[0].data.pipeline.commands[0].data.command.arguments[1].text.c_str());
}

void test_parser_if_else(void)
{
    ScriptParser parser;

    token_list_t tokens;
    tokens.push_back({"if", TokenType::IF});
    tokens.push_back({"cond", TokenType::VARIABLE});
    tokens.push_back({"then", TokenType::THEN});
    tokens.push_back({"echo", TokenType::WORD});
    tokens.push_back({"yes", TokenType::WORD});
    tokens.push_back({"else", TokenType::ELSE});
    tokens.push_back({"echo", TokenType::WORD});
    tokens.push_back({"no", TokenType::WORD});
    tokens.push_back({"fi", TokenType::FI});
    tokens.push_back({"", TokenType::END_OF_FILE});

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL_UINT(1, chain.size());
    TEST_ASSERT_EQUAL(ExpressionType::If, chain[0].tag);
    TEST_ASSERT_EQUAL(1, chain[0].data.ifExpr.conditions.size());
    TEST_ASSERT_EQUAL(ExpressionType::Condition, chain[0].data.ifExpr.conditions[0].tag);
    TEST_ASSERT_EQUAL(ExpressionType::Literal, chain[0].data.ifExpr.conditions[0].data.condition.conditionNodes[0].first.tag);
    TEST_ASSERT_EQUAL_STRING("cond", chain[0].data.ifExpr.conditions[0].data.condition.conditionNodes[0].first.data.literal.literalToken.text.c_str());
    TEST_ASSERT_EQUAL_UINT(1, chain[0].data.ifExpr.bodies.size());
    TEST_ASSERT_EQUAL_UINT(1, chain[0].data.ifExpr.bodies[0].size());
    TEST_ASSERT_EQUAL_UINT(1, chain[0].data.ifExpr.elseBody.size());
    assert_pipeline_expression(chain[0].data.ifExpr.bodies[0][0], "echo", 1);
    assert_pipeline_expression(chain[0].data.ifExpr.elseBody[0], "echo", 1);
    TEST_ASSERT_EQUAL_STRING("yes", chain[0].data.ifExpr.bodies[0][0].data.pipeline.commands[0].data.command.arguments[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("no", chain[0].data.ifExpr.elseBody[0].data.pipeline.commands[0].data.command.arguments[0].text.c_str());
}

void test_parser_empty_input(void)
{
    ScriptParser parser;

    token_list_t tokens;

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_TRUE(parser.error());
    TEST_ASSERT_EQUAL(ScriptParser::ErrorCode::EmptyInput, parser.getError());
    TEST_ASSERT_EQUAL_UINT(0, chain.size());
}

void test_parser_if_missing_condition_fails(void)
{
    ScriptParser parser;

    token_list_t tokens;
    tokens.push_back({"if", TokenType::IF});
    tokens.push_back({"then", TokenType::THEN});
    tokens.push_back({"echo", TokenType::WORD});
    tokens.push_back({"ok", TokenType::WORD});
    tokens.push_back({"fi", TokenType::FI});
    tokens.push_back({"", TokenType::END_OF_FILE});

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_TRUE(parser.error());
    TEST_ASSERT_EQUAL(ScriptParser::ErrorCode::EmptyCondition, parser.getError());
    // Even though the if statement is missing a condition, the parser returns a NIL expression for the if statement, which is considered a successfully parsed expression, so the chain size is 1
    TEST_ASSERT_EQUAL_UINT(1, chain.size());
}

void test_parser_if_condition_trailing_logical_operator_fails(void)
{
    ScriptParser parser;

    token_list_t tokens;
    tokens.push_back({"if", TokenType::IF});
    tokens.push_back({"cond", TokenType::VARIABLE});
    tokens.push_back({"&&", TokenType::AND_AND});
    tokens.push_back({"then", TokenType::THEN});
    tokens.push_back({"echo", TokenType::WORD});
    tokens.push_back({"ok", TokenType::WORD});
    tokens.push_back({"fi", TokenType::FI});
    tokens.push_back({"", TokenType::END_OF_FILE});

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_TRUE(parser.error());
    TEST_ASSERT_EQUAL(ScriptParser::ErrorCode::SyntaxError, parser.getError());
    TEST_ASSERT_EQUAL_UINT(1, chain.size());
}

void test_parser_while_missing_condition_fails(void)
{
    ScriptParser parser;

    token_list_t tokens;
    tokens.push_back({"while", TokenType::WHILE});
    tokens.push_back({"do", TokenType::DO});
    tokens.push_back({"echo", TokenType::WORD});
    tokens.push_back({"loop", TokenType::WORD});
    tokens.push_back({"done", TokenType::DONE});
    tokens.push_back({"", TokenType::END_OF_FILE});

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_TRUE(parser.error());
    TEST_ASSERT_EQUAL(ScriptParser::ErrorCode::EmptyCondition, parser.getError());
    // Even though the while loop is missing a condition, the parser returns a NIL expression for the while loop, which is considered a successfully parsed expression, so the chain size is 1
    TEST_ASSERT_EQUAL_UINT(1, chain.size());
}

void test_parser_while_empty_body_fails(void)
{
    ScriptParser parser;

    token_list_t tokens;
    tokens.push_back({"while", TokenType::WHILE});
    tokens.push_back({"cond", TokenType::VARIABLE});
    tokens.push_back({"do", TokenType::DO});
    tokens.push_back({"done", TokenType::DONE});
    tokens.push_back({"", TokenType::END_OF_FILE});

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_TRUE(parser.error());
    TEST_ASSERT_EQUAL(ScriptParser::ErrorCode::EmptyWhileBody, parser.getError());
    // Even though the while loop is missing a body, the parser returns a NIL expression for the while loop, which is considered a successfully parsed expression, so the chain size is 1
    TEST_ASSERT_EQUAL_UINT(1, chain.size());
}

void test_parser_for_missing_values_fails(void)
{
    ScriptParser parser;

    token_list_t tokens;
    tokens.push_back({"for", TokenType::FOR});
    tokens.push_back({"i", TokenType::WORD});
    tokens.push_back({"=", TokenType::EQUALS});
    tokens.push_back({"do", TokenType::DO});
    tokens.push_back({"echo", TokenType::WORD});
    tokens.push_back({"x", TokenType::WORD});
    tokens.push_back({"done", TokenType::DONE});
    tokens.push_back({"", TokenType::END_OF_FILE});

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_TRUE(parser.error());
    TEST_ASSERT_EQUAL(ScriptParser::ErrorCode::EmptyForLoopValues, parser.getError());
    // Even though the for loop is missing values, the parser returns a NIL expression for the for loop, which is considered a successfully parsed expression, so the chain size is 1
    TEST_ASSERT_EQUAL_UINT(1, chain.size());
}

void test_parser_for_empty_body_fails(void)
{
    ScriptParser parser;

    token_list_t tokens;
    tokens.push_back({"for", TokenType::FOR});
    tokens.push_back({"i", TokenType::WORD});
    tokens.push_back({"=", TokenType::EQUALS});
    tokens.push_back({"1", TokenType::WORD});
    tokens.push_back({"do", TokenType::DO});
    tokens.push_back({"done", TokenType::DONE});
    tokens.push_back({"", TokenType::END_OF_FILE});

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_TRUE(parser.error());
    TEST_ASSERT_EQUAL(ScriptParser::ErrorCode::EmptyForLoopBody, parser.getError());
    // Even though the for loop is missing a body, the parser returns a NIL expression for the for loop, which is considered a successfully parsed expression, so the chain size is 1
    TEST_ASSERT_EQUAL_UINT(1, chain.size());
}

void test_parser_for_loop_with_multiple_values(void)
{
    ScriptParser parser;

    Lexer lexer;
    token_list_t tokens = lexer.tokenize("for i = 1 2 3; do echo $i done");

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL_UINT(1, chain.size());
    TEST_ASSERT_EQUAL((int)ExpressionType::ForLoop, (int)chain[0].tag);
    TEST_ASSERT_EQUAL_STRING("i", chain[0].data.forLoop.variable.c_str());
    TEST_ASSERT_EQUAL_UINT(3, chain[0].data.forLoop.values.size());
    TEST_ASSERT_EQUAL_STRING("1", chain[0].data.forLoop.values[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("2", chain[0].data.forLoop.values[1].text.c_str());
    TEST_ASSERT_EQUAL_STRING("3", chain[0].data.forLoop.values[2].text.c_str());
}

void test_parser_function_definition_and_call(void)
{
    ScriptParser parser;

    Lexer lexer;
    token_list_t tokens = lexer.tokenize("function myFunc(i j) echo $i $j done\nmyFunc(1 2)");

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL_UINT(2, chain.size());
    TEST_ASSERT_EQUAL(ExpressionType::FunctionDefinition, chain[0].tag);
    TEST_ASSERT_EQUAL_STRING("myFunc", chain[0].data.functionDef.functionName.c_str());
    TEST_ASSERT_EQUAL_UINT(2, chain[0].data.functionDef.parameters.size());
    TEST_ASSERT_EQUAL_STRING("i", chain[0].data.functionDef.parameters[0].c_str());
    TEST_ASSERT_EQUAL_STRING("j", chain[0].data.functionDef.parameters[1].c_str());
    TEST_ASSERT_EQUAL_UINT(1, chain[0].data.functionDef.body.size());
    assert_pipeline_expression(chain[0].data.functionDef.body[0], "echo", 2);
    TEST_ASSERT_EQUAL(1, chain[0].data.functionDef.body[0].data.pipeline.commands.size());
    TEST_ASSERT_EQUAL_STRING("echo", chain[0].data.functionDef.body[0].data.pipeline.commands[0].data.command.keyword.text.c_str());
    TEST_ASSERT_EQUAL_STRING("i", chain[0].data.functionDef.body[0].data.pipeline.commands[0].data.command.arguments[0].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::VARIABLE, chain[0].data.functionDef.body[0].data.pipeline.commands[0].data.command.arguments[0].type);
    TEST_ASSERT_EQUAL_STRING("j", chain[0].data.functionDef.body[0].data.pipeline.commands[0].data.command.arguments[1].text.c_str());
    TEST_ASSERT_EQUAL(TokenType::VARIABLE, chain[0].data.functionDef.body[0].data.pipeline.commands[0].data.command.arguments[1].type);

    TEST_ASSERT_EQUAL(ExpressionType::FunctionCall, chain[1].tag);
    TEST_ASSERT_EQUAL_STRING("myFunc", chain[1].data.functionCall.functionName.c_str());
    TEST_ASSERT_EQUAL_UINT(2, chain[1].data.functionCall.arguments.size());
    TEST_ASSERT_EQUAL_STRING("1", chain[1].data.functionCall.arguments[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("2", chain[1].data.functionCall.arguments[1].text.c_str());
}

void test_parser_simple_commands_concatenated_with_newline_or_semicolon(void)
{
    ScriptParser parser;

    Lexer lexer;
    token_list_t tokens = lexer.tokenize("echo hello\n echo world; echo again");

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL_UINT(3, chain.size());
    assert_pipeline_expression(chain[0], "echo", 1);
    assert_pipeline_expression(chain[1], "echo", 1);
    assert_pipeline_expression(chain[2], "echo", 1);
    TEST_ASSERT_EQUAL_STRING("hello", chain[0].data.pipeline.commands[0].data.command.arguments[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("world", chain[1].data.pipeline.commands[0].data.command.arguments[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("again", chain[2].data.pipeline.commands[0].data.command.arguments[0].text.c_str());
}

void test_parser_function_call(void)
{
    ScriptParser parser;

    Lexer lexer;
    token_list_t tokens = lexer.tokenize("myFunc(1 2;3)");

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL_UINT(1, chain.size());
    TEST_ASSERT_EQUAL(ExpressionType::FunctionCall, chain[0].tag);
    TEST_ASSERT_EQUAL_STRING("myFunc", chain[0].data.functionCall.functionName.c_str());
    TEST_ASSERT_EQUAL_UINT(3, chain[0].data.functionCall.arguments.size());
    TEST_ASSERT_EQUAL_STRING("1", chain[0].data.functionCall.arguments[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("2", chain[0].data.functionCall.arguments[1].text.c_str());
    TEST_ASSERT_EQUAL_STRING("3", chain[0].data.functionCall.arguments[2].text.c_str());
}

void test_parser_nested_control_flow(void)
{
    ScriptParser parser;
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("if true then while $cond do echo nested done else echo fallback fi");

    size_t index = 0;
    ExpressionChain chain = parser.parse(tokens, index);

    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL_UINT(1, chain.size());
    TEST_ASSERT_EQUAL((int)ExpressionType::If, (int)chain[0].tag);
    TEST_ASSERT_EQUAL_UINT(1, chain[0].data.ifExpr.bodies.size());
    TEST_ASSERT_EQUAL_UINT(1, chain[0].data.ifExpr.bodies[0].size());
    TEST_ASSERT_EQUAL((int)ExpressionType::WhileLoop, (int)chain[0].data.ifExpr.bodies[0][0].tag);
    TEST_ASSERT_EQUAL_UINT(1, chain[0].data.ifExpr.elseBody.size());
    assert_pipeline_expression(chain[0].data.ifExpr.elseBody[0], "echo", 1);
}

void test_parser_rejects_invalid_function_argument(void)
{
    ScriptParser parser;
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("function bad(x &&) echo done");

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_TRUE(parser.error());
    TEST_ASSERT_EQUAL(ScriptParser::ErrorCode::InvalidFunctionArgument, parser.getError());
    TEST_ASSERT_EQUAL_UINT(1, chain.size());
    TEST_ASSERT_EQUAL((int)ExpressionType::Nil, (int)chain[0].tag);
}

void test_parser_function_call_with_arguments_and_spacing(void)
{
    ScriptParser parser;
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("myFunc(1\n 2; 3)");

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL_UINT(1, chain.size());
    TEST_ASSERT_EQUAL((int)ExpressionType::FunctionCall, (int)chain[0].tag);
    TEST_ASSERT_EQUAL_UINT(3, chain[0].data.functionCall.arguments.size());
    TEST_ASSERT_EQUAL_STRING("1", chain[0].data.functionCall.arguments[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("2", chain[0].data.functionCall.arguments[1].text.c_str());
    TEST_ASSERT_EQUAL_STRING("3", chain[0].data.functionCall.arguments[2].text.c_str());
}

void test_parser_every_command_is_a_pipeline(void)
{
    ScriptParser parser;
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("echo hello | echo world");

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL_UINT(1, chain.size());
    assert_pipeline_expression(chain[0], "echo", 1);
    TEST_ASSERT_EQUAL((int)ExpressionType::Pipeline, (int)chain[0].tag);
    TEST_ASSERT_EQUAL_UINT(2, chain[0].data.pipeline.commands.size());
    TEST_ASSERT_EQUAL_STRING("hello", chain[0].data.pipeline.commands[0].data.command.arguments[0].text.c_str());
    TEST_ASSERT_EQUAL_STRING("world", chain[0].data.pipeline.commands[1].data.command.arguments[0].text.c_str());
}

void test_parser_build_pipeline_set_index_correctly(void)
{
    ScriptParserHarness parser;
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("echo hello | echo world; echo again");

    size_t index = 0;
    Expression pipelineExpr = parser.buildPipelineExpression(tokens, index, {TokenType::SEMI, TokenType::END_OF_FILE});

    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL((int)ExpressionType::Pipeline, (int)pipelineExpr.tag);
    TEST_ASSERT_EQUAL(5, index); // The index should be at the position of the semicolon after parsing the pipeline expression, which is token index 5 in this case (0-based index)
}

void test_parser_hand_picked_inputs(void)
{
    ScriptParser parser;
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("run; collect done");

    size_t index = 0;
    ExpressionChain chain = parser.parse(tokens, index);

    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL_UINT(2, chain.size());
    TEST_ASSERT_EQUAL((int)ExpressionType::Pipeline, (int)chain[0].tag);
    TEST_ASSERT_EQUAL_UINT(1, chain[0].data.pipeline.commands.size());
    TEST_ASSERT_EQUAL_STRING("run", chain[0].data.pipeline.commands[0].data.command.keyword.text.c_str());
    TEST_ASSERT_EQUAL((int)ExpressionType::Pipeline, (int)chain[1].tag);
    TEST_ASSERT_EQUAL_UINT(1, chain[1].data.pipeline.commands.size());
    TEST_ASSERT_EQUAL(1, chain[1].data.pipeline.commands[0].data.command.arguments.size());
    TEST_ASSERT_EQUAL_STRING("collect", chain[1].data.pipeline.commands[0].data.command.keyword.text.c_str());
    TEST_ASSERT_EQUAL_STRING("done", chain[1].data.pipeline.commands[0].data.command.arguments[0].text.c_str());

    auto tokens2 = lexer.tokenize("test extra\n\r");
    ExpressionChain chain2 = parser.parse(tokens2);
    TEST_ASSERT_FALSE(parser.error());

    auto tokens3 = lexer.tokenize("test = true; while $test do mycmd; test= false; done");
    ExpressionChain chain3 = parser.parse(tokens3);
    TEST_ASSERT_FALSE(parser.error());
}

void test_parser_for_loop_parsing_returns_unexpected_end_of_input(void)
{
    ScriptParser parser;
    Lexer lexer;

    ETVector<ETString> inputs = {
        "for i = 1 2 3; do echo $i",   // Missing "done" at the end of the for loop
        "for i = 1 2 3; do echo $i;",  // Missing "done" at the end of the for loop, even though there is a semicolon at the end, it's still an incomplete for loop
        "for i = 1 2 3; do echo $i\n", // Missing "done" at the end of the for loop, even though there is a newline at the end, it's still an incomplete for loop
        "for i = 1 2 3\n",             // Missing "do" and "done" for the for loop, which makes it an incomplete for loop
        "for i = 1 2 3; do\n",         // Missing the body of the for loop and the "done" keyword, which makes it an incomplete for loop
        "for \n",                      // Incomplete for loop with only the "for" keyword and nothing else
    };

    for (const auto &input : inputs)
    {
        TEST_MESSAGE(("Testing input: " + input).c_str());
        token_list_t tokens = lexer.tokenize(input);
        ExpressionChain chain = parser.parse(tokens);
        TEST_ASSERT_TRUE(parser.error());
        TEST_ASSERT_EQUAL(ScriptParser::ErrorCode::UnexpectedEndOfInput, parser.getError());
        TEST_ASSERT_EQUAL_UINT(1, chain.size());
        TEST_ASSERT_EQUAL((int)ExpressionType::Nil, (int)chain[0].tag);
    }
}

void test_parser_if_parsing_returns_unexpected_end_of_input(void)
{
    ScriptParser parser;
    Lexer lexer;

    ETVector<ETString> inputs = {
        "if $cond",                // Missing "then" and the rest of the if expression
        "if $cond;",               // Missing "then" even though there is a separator at the end
        "if $cond\n",              // Missing "then" even though there is a newline at the end
        "if $cond then echo yes",  // Missing "fi" at the end of the if expression
        "if $cond then echo yes;", // Missing "fi" at the end of the if expression, even though there is a semicolon at the end
    };

    for (const auto &input : inputs)
    {
        TEST_MESSAGE(("Testing input: " + input).c_str());
        token_list_t tokens = lexer.tokenize(input);
        ExpressionChain chain = parser.parse(tokens);
        TEST_ASSERT_TRUE(parser.error());
        TEST_ASSERT_EQUAL(ScriptParser::ErrorCode::UnexpectedEndOfInput, parser.getError());
        TEST_ASSERT_EQUAL_UINT(1, chain.size());
        TEST_ASSERT_EQUAL((int)ExpressionType::Nil, (int)chain[0].tag);
    }
}

void test_parser_while_parsing_returns_unexpected_end_of_input(void)
{
    ScriptParser parser;
    Lexer lexer;

    ETVector<ETString> inputs = {
        "while $cond",                  // Missing "do" and the rest of the while expression
        "while $cond;",                 // Missing "do" even though there is a separator at the end
        "while $cond\n",                // Missing "do" even though there is a newline at the end
        "while $cond do echo looping",  // Missing "done" at the end of the while expression
        "while $cond do echo looping;", // Missing "done" at the end of the while expression, even though there is a semicolon at the end
    };

    for (const auto &input : inputs)
    {
        TEST_MESSAGE(("Testing input: " + input).c_str());
        token_list_t tokens = lexer.tokenize(input);
        ExpressionChain chain = parser.parse(tokens);
        TEST_ASSERT_TRUE(parser.error());
        TEST_ASSERT_EQUAL(ScriptParser::ErrorCode::UnexpectedEndOfInput, parser.getError());
        TEST_ASSERT_EQUAL_UINT(1, chain.size());
        TEST_ASSERT_EQUAL((int)ExpressionType::Nil, (int)chain[0].tag);
    }
}

void test_parser_function_definition_parsing_returns_unexpected_end_of_input(void)
{
    ScriptParser parser;
    Lexer lexer;

    ETVector<ETString> inputs = {
        "function myFunc",               // Missing the parameter list and the body of the function definition
        "function myFunc(",              // Missing the parameter list and the body of the function definition, even though there is an opening parenthesis
        "function myFunc(\n",            // Missing the parameter list and the body of the function definition, even though there is a closing parenthesis and a newline at the end
        "function myFunc() echo hello",  // Missing "done" at the end of the function definition
        "function myFunc() echo hello;", // Missing "done" at the end of the function definition, even though there is a semicolon at the end
    };

    for (const auto &input : inputs)
    {
        TEST_MESSAGE(("Testing input: " + input).c_str());
        token_list_t tokens = lexer.tokenize(input);
        ExpressionChain chain = parser.parse(tokens);
        TEST_ASSERT_TRUE(parser.error());
        TEST_ASSERT_EQUAL(ScriptParser::ErrorCode::UnexpectedEndOfInput, parser.getError());
        TEST_ASSERT_EQUAL_UINT(1, chain.size());
        TEST_ASSERT_EQUAL((int)ExpressionType::Nil, (int)chain[0].tag);
    }
}

void test_parser_function_call_parsing_returns_unexpected_end_of_input(void)
{
    ScriptParser parser;
    Lexer lexer;

    ETVector<ETString> inputs = {
        "myFunc(",      // Missing the arguments and the closing parenthesis for the function call
        "myFunc(1 2",   // Missing the closing parenthesis for the function call, even though there are arguments
        "myFunc(1 2;",  // Missing the closing parenthesis for the function call, even though there are arguments and a semicolon at the end
        "myFunc(1 2\n", // Missing the closing parenthesis for the function call, even though there are arguments and a newline at the end
    };

    for (const auto &input : inputs)
    {
        TEST_MESSAGE(("Testing input: " + input).c_str());
        token_list_t tokens = lexer.tokenize(input);
        ExpressionChain chain = parser.parse(tokens);
        TEST_ASSERT_TRUE(parser.error());
        TEST_ASSERT_EQUAL(ScriptParser::ErrorCode::UnexpectedEndOfInput, parser.getError());
        TEST_ASSERT_EQUAL_UINT(1, chain.size());
        TEST_ASSERT_EQUAL((int)ExpressionType::Nil, (int)chain[0].tag);
    }
}

void test_parser_deeply_nested_blocks(void)
{
    ScriptParser parser;
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("if true then if true then if true then echo nested fi fi fi");

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL_UINT(1, chain.size());
    TEST_ASSERT_EQUAL((int)ExpressionType::If, (int)chain[0].tag);
}

void test_parser_malformed_unmatched_quotes_rejects(void)
{
    ScriptParser parser;
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("echo \"hello world");

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_TRUE(parser.error());
}

void test_parser_large_argument_list_accepted(void)
{
    ScriptParser parser;
    Lexer lexer;

    ETString largeScript = "echo";
    for (int i = 0; i < 100; i++)
    {
        largeScript += " arg";
        largeScript += std::to_string(i).c_str();
    }

    token_list_t tokens = lexer.tokenize(largeScript);
    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL_UINT(1, chain.size());
    TEST_ASSERT_EQUAL_UINT(100, chain[0].data.pipeline.commands[0].data.command.arguments.size());
}

void test_parser_empty_for_loop_body_rejects(void)
{
    ScriptParser parser;
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("for i = 1 2; do ; done");

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_TRUE(parser.error());
}

void test_parser_invalid_variable_names_in_assignment(void)
{
    ScriptParser parser;
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("123invalid = value");

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_FALSE(parser.error());
}

void test_parser_mixed_control_flow_structures(void)
{
    ScriptParser parser;
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("while true do for i = 1 2; do echo $i done done");

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL_UINT(1, chain.size());
    TEST_ASSERT_EQUAL((int)ExpressionType::WhileLoop, (int)chain[0].tag);
    TEST_ASSERT_EQUAL_UINT(1, chain[0].data.whileLoop.body.size());
    TEST_ASSERT_EQUAL((int)ExpressionType::ForLoop, (int)chain[0].data.whileLoop.body[0].tag);
}

void test_parser_function_with_no_parameters(void)
{
    ScriptParser parser;
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("function noParams() echo test done\nnoParams()");

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_FALSE(parser.error());
    TEST_ASSERT_EQUAL_UINT(2, chain.size());
    TEST_ASSERT_EQUAL((int)ExpressionType::FunctionDefinition, (int)chain[0].tag);
    TEST_ASSERT_EQUAL_UINT(0, chain[0].data.functionDef.parameters.size());
}

void test_parser_escaped_special_characters(void)
{
    ScriptParser parser;
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("echo hello\\;world");

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_FALSE(parser.error());
}

void test_parser_rejects_multiple_assignment_operators(void)
{
    ScriptParser parser;
    Lexer lexer;
    token_list_t tokens = lexer.tokenize("x = = 5");

    ExpressionChain chain = parser.parse(tokens);

    TEST_ASSERT_TRUE(parser.error());
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_parser_simple_command);
    RUN_TEST(test_parser_simple_commands_concatenated_with_newline_or_semicolon);
    RUN_TEST(test_parser_function_call);
    RUN_TEST(test_parser_nested_control_flow);
    RUN_TEST(test_parser_rejects_invalid_function_argument);
    RUN_TEST(test_parser_function_call_with_arguments_and_spacing);
    RUN_TEST(test_parser_while_loop);
    RUN_TEST(test_parser_for_loop_with_script_command);
    RUN_TEST(test_parser_if_else);
    RUN_TEST(test_parser_empty_input);
    RUN_TEST(test_parser_if_missing_condition_fails);
    RUN_TEST(test_parser_if_condition_trailing_logical_operator_fails);
    RUN_TEST(test_parser_while_missing_condition_fails);
    RUN_TEST(test_parser_while_empty_body_fails);
    RUN_TEST(test_parser_for_missing_values_fails);
    RUN_TEST(test_parser_for_empty_body_fails);
    RUN_TEST(test_parser_for_loop_with_multiple_values);
    RUN_TEST(test_parser_function_definition_and_call);
    RUN_TEST(test_parser_every_command_is_a_pipeline);
    RUN_TEST(test_parser_build_pipeline_set_index_correctly);
    RUN_TEST(test_parser_hand_picked_inputs);
    RUN_TEST(test_parser_for_loop_parsing_returns_unexpected_end_of_input);
    RUN_TEST(test_parser_if_parsing_returns_unexpected_end_of_input);
    RUN_TEST(test_parser_while_parsing_returns_unexpected_end_of_input);
    RUN_TEST(test_parser_function_definition_parsing_returns_unexpected_end_of_input);
    RUN_TEST(test_parser_function_call_parsing_returns_unexpected_end_of_input);
    RUN_TEST(test_parser_deeply_nested_blocks);
    RUN_TEST(test_parser_malformed_unmatched_quotes_rejects);
    RUN_TEST(test_parser_large_argument_list_accepted);
    RUN_TEST(test_parser_empty_for_loop_body_rejects);
    RUN_TEST(test_parser_invalid_variable_names_in_assignment);
    RUN_TEST(test_parser_mixed_control_flow_structures);
    RUN_TEST(test_parser_function_with_no_parameters);
    RUN_TEST(test_parser_escaped_special_characters);
    RUN_TEST(test_parser_rejects_multiple_assignment_operators);
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
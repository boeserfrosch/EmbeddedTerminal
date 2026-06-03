#include "Parser.h"
// #include "Validator.h"
#include "FeatureFlags.h"
#include "lang/LangAPI.h"
// #include "exec/ShellParser.h"
#include "lang/TokenUtils.h"

using namespace EmbeddedTerminal::Lang;
using namespace EmbeddedTerminal::Scripting;

bool isValidArgumentToken(TokenType type)
{
    return type == TokenType::WORD || type == TokenType::DOUBLE_QUOTE_STRING_LITERAL || type == TokenType::SINGLE_QUOTE_STRING_LITERAL || type == TokenType::TRUE || type == TokenType::FALSE || type == TokenType::VARIABLE;
}

bool isValidKeywordToken(TokenType type)
{
    return type == TokenType::WORD || isReservedWordToken(type);
}

bool isValidCommandArgumentToken(TokenType type)
{
    return isValidArgumentToken(type) || type == TokenType::REDIR_OUT || type == TokenType::REDIR_IN || type == TokenType::REDIR_APPEND || isReservedWordToken(type);
}

bool isValidForLoopValueToken(TokenType type)
{
    return isValidArgumentToken(type);
}

bool findFirstIndexOfTokenType(const token_list_t &tokens, TokenType type, size_t &startIndex)
{
    for (size_t i = startIndex; i < tokens.size(); ++i)
    {
        if (tokens[i].type == type)
        {
            startIndex = i;
            return true;
        }
    }
    return false; // Return false to indicate not found
}

ScriptParser::ErrorCode expected(const token_list_t &tokens, size_t index, TokenType type, ScriptParser::ErrorCode errorCode = ScriptParser::ErrorCode::SyntaxError)
{
    if (index >= tokens.size() || tokens[index].type == TokenType::END_OF_FILE)
    {
        return ScriptParser::ErrorCode::UnexpectedEndOfInput;
    }

    if (tokens[index].type != type)
    {
        return errorCode;
    }
    return ScriptParser::ErrorCode::None;
};

size_t getNextMeaningfulTokenIndex(const token_list_t &tokens, size_t startIndex)
{
    size_t nextIndex = startIndex;
    while (nextIndex < tokens.size() && isSeparator(tokens[nextIndex].type))
    {
        ++nextIndex;
    }
    return nextIndex;
};

void Expression::destroy()
{
    switch (tag)
    {
    case ExpressionType::Command:
        data.command.~CommandExpression();
        break;
    case ExpressionType::If:
        data.ifExpr.~IfExpression();
        break;
    case ExpressionType::WhileLoop:
        data.whileLoop.~WhileLoopExpression();
        break;
    case ExpressionType::ForLoop:
        data.forLoop.~ForLoopExpression();
        break;
    case ExpressionType::Nil:
        data.nil.~NilExpression();
        break;
    case ExpressionType::FunctionDefinition:
        data.functionDef.~FunctionDefinitionExpression();
        break;
    case ExpressionType::FunctionCall:
        data.functionCall.~FunctionCallExpression();
        break;
    case ExpressionType::VariableAssignment:
        data.variableAssignment.~VariableAssignmentExpression();
        break;
    case ExpressionType::Pipeline:
        data.pipeline.~PipelineExpression();
        break;
    case ExpressionType::Condition:
        data.condition.~ConditionExpression();
        break;
    case ExpressionType::Literal:
        data.literal.~LiteralExpression();
        break;
    case ExpressionType::Comparison:
        data.comparison.~ComparisonExpression();
        break;
    }

    tag = ExpressionType::Nil;
    new (&data.nil) NilExpression();
}

void Expression::copyFrom(const Expression &other)
{
    tag = other.tag;

    switch (other.tag)
    {
    case ExpressionType::Command:
        new (&data.command) CommandExpression();
        data.command.keyword = other.data.command.keyword;
        data.command.arguments = other.data.command.arguments;
        data.command.hasRedirectIn = other.data.command.hasRedirectIn;
        data.command.hasRedirectOut = other.data.command.hasRedirectOut;
        data.command.appendRedirect = other.data.command.appendRedirect;
        data.command.redirectInPath = other.data.command.redirectInPath;
        data.command.redirectOutPath = other.data.command.redirectOutPath;
        break;
    case ExpressionType::If:
        new (&data.ifExpr) IfExpression(other.data.ifExpr);
        break;
    case ExpressionType::WhileLoop:
        new (&data.whileLoop) WhileLoopExpression();
        data.whileLoop.condition = std::make_unique<Expression>(*other.data.whileLoop.condition);
        data.whileLoop.body = other.data.whileLoop.body;
        break;
    case ExpressionType::ForLoop:
        new (&data.forLoop) ForLoopExpression(other.data.forLoop);
        break;
    case ExpressionType::Nil:
        new (&data.nil) NilExpression();
        break;
    case ExpressionType::FunctionDefinition:
        new (&data.functionDef) FunctionDefinitionExpression(other.data.functionDef);
        break;
    case ExpressionType::FunctionCall:
        new (&data.functionCall) FunctionCallExpression(other.data.functionCall);
        break;
    case ExpressionType::VariableAssignment:
        new (&data.variableAssignment) VariableAssignmentExpression();
        data.variableAssignment.variable = other.data.variableAssignment.variable;
        if (other.data.variableAssignment.valueExpression)
        {
            data.variableAssignment.valueExpression = std::make_unique<Expression>(*other.data.variableAssignment.valueExpression);
        }
        break;
    case ExpressionType::Pipeline:
        new (&data.pipeline) PipelineExpression(other.data.pipeline);
        break;
    case ExpressionType::Condition:
        new (&data.condition) ConditionExpression(other.data.condition);
        break;
    case ExpressionType::Literal:
        new (&data.literal) LiteralExpression(other.data.literal);
        break;
    case ExpressionType::Comparison:
        new (&data.comparison) ComparisonExpression();
        if (other.data.comparison.leftOperand)
        {
            data.comparison.leftOperand = std::make_unique<Expression>(*other.data.comparison.leftOperand);
        }
        data.comparison.operatorType = other.data.comparison.operatorType;
        if (other.data.comparison.rightOperand)
        {
            data.comparison.rightOperand = std::make_unique<Expression>(*other.data.comparison.rightOperand);
        }
        break;
    }
}

Expression Expression::createCommand(const token_t &keyword, const token_list_t &arguments, bool hasRedirectIn, bool hasRedirectOut, bool appendRedirect, const token_t &redirectInPath, const token_t &redirectOutPath)
{
    Expression expression;
    expression.destroy();
    expression.tag = ExpressionType::Command;
    token_list_t ownedArguments = arguments;
    new (&expression.data.command) CommandExpression{keyword, ownedArguments, hasRedirectIn, hasRedirectOut, appendRedirect, redirectInPath, redirectOutPath};
    return expression;
}

Expression Expression::createIfExpression(const ETVector<Expression> &conditions, const ETVector<ExpressionChain> &bodies, const ETVector<Expression> &elseBody)
{
    Expression expression;
    expression.destroy();
    expression.tag = ExpressionType::If;
    new (&expression.data.ifExpr) IfExpression{conditions, bodies, elseBody};
    return expression;
}

Expression Expression::createWhileLoop(const Expression &cond, const ETVector<Expression> &body)
{
    Expression expression;
    expression.destroy();
    expression.tag = ExpressionType::WhileLoop;
    new (&expression.data.whileLoop) WhileLoopExpression{std::make_unique<Expression>(cond), body};
    return expression;
}

Expression Expression::createForLoop(const TokenText &variable, const token_list_t &values, const ETVector<Expression> &body)
{
    Expression expression;
    expression.destroy();
    expression.tag = ExpressionType::ForLoop;
    token_list_t ownedValues = values;
    new (&expression.data.forLoop) ForLoopExpression{variable, ownedValues, body};
    return expression;
}

Expression Expression::createFunctionDefinition(const TokenText &functionName, const ETVector<TokenText> &parameters, const ETVector<Expression> &body)
{
    Expression expression;
    expression.destroy();
    expression.tag = ExpressionType::FunctionDefinition;
    new (&expression.data.functionDef) FunctionDefinitionExpression{functionName, parameters, body};
    return expression;
}

Expression Expression::createFunctionCall(const TokenText &functionName, const token_list_t &arguments)
{
    Expression expression;
    expression.destroy();
    expression.tag = ExpressionType::FunctionCall;
    token_list_t ownedArguments = arguments;
    // for (auto &argument : ownedArguments)
    // {
    //     materializeTokenText_(argument);
    // }
    new (&expression.data.functionCall) FunctionCallExpression{functionName, ownedArguments};
    return expression;
}

Expression EmbeddedTerminal::Scripting::Expression::createVariableAssignment(const TokenText &variable, const Expression &valueExpression)
{
    Expression expression;
    expression.destroy();
    expression.tag = ExpressionType::VariableAssignment; // We can treat variable assignment as a special type of command expression during execution, since it has a similar structure with a keyword (the variable name) and arguments (the value tokens)
    new (&expression.data.variableAssignment) VariableAssignmentExpression{variable, std::make_unique<Expression>(valueExpression)};
    return expression;
}

Expression EmbeddedTerminal::Scripting::Expression::createPipeline(const ETVector<Expression> &commands)
{
    Expression expression;
    expression.destroy();
    expression.tag = ExpressionType::Pipeline;
    new (&expression.data.pipeline) PipelineExpression{commands};
    return expression;
}

Expression Expression::createNil()
{
    Expression expression;
    expression.destroy();
    expression.tag = ExpressionType::Nil;
    new (&expression.data.nil) NilExpression();
    return expression;
}

Expression EmbeddedTerminal::Scripting::Expression::createConditionExpression(const ETVector<ETPair<Expression, bool>> &conditionNodes, const ETVector<TokenType> &connectingOperators)
{
    Expression expression;
    expression.destroy();
    expression.tag = ExpressionType::Condition;
    new (&expression.data.condition) ConditionExpression{conditionNodes, connectingOperators};
    return expression;
}

Expression EmbeddedTerminal::Scripting::Expression::createLiteral(const token_t &literalToken)
{
    Expression expression;
    expression.destroy();
    expression.tag = ExpressionType::Literal;
    new (&expression.data.literal) LiteralExpression{literalToken};
    return expression;
}

Expression EmbeddedTerminal::Scripting::Expression::createComparison(const Expression &leftOperand, TokenType operatorType, const Expression &rightOperand)
{
    Expression expression;
    expression.destroy();
    expression.tag = ExpressionType::Comparison;
    new (&expression.data.comparison) ComparisonExpression{std::make_unique<Expression>(leftOperand), operatorType, std::make_unique<Expression>(rightOperand)};
    return expression;
}

Expression::Expression() : tag(ExpressionType::Nil)
{
    new (&data.nil) NilExpression();
}

Expression::~Expression()
{
    destroy();
}

Expression::Expression(const Expression &other) : tag(ExpressionType::Nil)
{
    new (&data.nil) NilExpression();
    copyFrom(other);
}

Expression &Expression::operator=(const Expression &other)
{
    if (this != &other)
    {
        destroy();
        copyFrom(other);
    }
    return *this;
}

ExpressionChain ScriptParser::parse(const token_list_t &tokens)
{
    size_t startIndex = 0;
    return parse(tokens, startIndex);
}

ExpressionChain ScriptParser::parse(const token_list_t &tokens, size_t &startIndex)
{
    if (tokens.empty())
    {
        error_ = ScriptParser::ErrorCode::EmptyInput;
        return ExpressionChain();
    }

    while (startIndex < tokens.size() && isSeparator(tokens[startIndex].type))
    {
        ++startIndex; // Skip leading separators to find the first meaningful token. This allows the parser to handle scripts that start with newlines or semicolons without treating them as syntax errors, and it also simplifies the parsing logic by ensuring that we always start parsing with a non-separator token.
    }

    return parseExpressionChainUntilToken(tokens, startIndex, TokenType::END_OF_FILE);
}

ExpressionChain ScriptParser::parseExpressionChainUntilToken(const token_list_t &tokens, size_t &startIndex, TokenType stopAt)
{
    return parseExpressionChainUntilToken(tokens, startIndex, ETVector<TokenType>{stopAt});
}

ExpressionChain ScriptParser::parseExpressionChainUntilToken(const token_list_t &tokens, size_t &startIndex, const ETVector<TokenType> &stopAt)
{
    Scripting::ExpressionChain chain;

    size_t index = startIndex;
    for (; index < tokens.size(); ++index)
    {
        TokenType currentType = tokens[index].type;
        // Is it a stopAt token? If so, we should stop parsing and return the chain we have so far. We check this at the beginning of the loop because if the first token is a stopAt token, we should return an empty chain and let the caller handle it, rather than treating it as an unexpected token.
        if (isStopToken(currentType, stopAt))
        {
            startIndex = index; // Update the startIndex to the current index where we encountered the stop token, so that the caller can know where the parsing stopped when we return
            return chain;
        }

        // Shall we ignore separators? What is if stopAt is a separator itself? For now, let's just ignore separators and not treat them as expressions in the chain
        if (isSeparator(currentType))
        {
            continue;
        }

        switch (currentType)
        {
        case TokenType::WORD:
            // There could be three cases
            // 1. It's a regular command with arguments
            // 2. It's a function call, in which case the next token should be a PAREN_OPEN. For simplicity, let's assume that function calls are always of the form functionName(arg1 arg2 ...)
            // 3. It's a variable assignment, in which case the next token should be an EQUALS and the token after that should be the value to assign to the variable.
            if ((index + 1) < tokens.size() && tokens[index + 1].type == TokenType::PAREN_OPEN)
            {
                // It's a function call
                chain.push_back(buildFunctionCallExpression(tokens, index));
                if (error()) // If there was an error while building the function call expression, we should stop parsing and return what we have so far
                {
                    startIndex = index; // Update the startIndex to the current index where the error occurred, so that the caller can know where the parsing stopped
                    return chain;
                }
                error_ = ScriptParser::ErrorCode::None;
            }
            else if ((index + 1) < tokens.size() && tokens[index + 1].type == TokenType::EQUALS)
            {
                // It is a variable assignment.
                chain.push_back(buildVariableAssignmentExpression(tokens, index, stopAt));
                if (error())
                {
                    startIndex = index; // Update the startIndex to the current index where the error occurred, so that the caller can know where the parsing stopped
                    return chain;
                }
                error_ = ScriptParser::ErrorCode::None;
            }
            else
            {
                // It's a regular command
                auto expression = buildPipelineExpression(tokens, index, stopAt);
                chain.push_back(expression);
                if (error()) // If there was an error while building the command expression, we should stop parsing and return what we have so far
                {
                    startIndex = index; // Update the startIndex to the current index where the error occurred, so that the caller can know where the parsing stopped
                    return chain;
                }
                error_ = ScriptParser::ErrorCode::None;
            }

            break;
        case TokenType::WHILE:
#if ET_ENABLE_SCRIPT_WHILE
            chain.push_back(buildWhileLoopExpression(tokens, index));
            if (error())
            {
                startIndex = index; // Update the startIndex to the current index where the error occurred, so that the caller can know where the parsing stopped
                return chain;
            }
            error_ = ScriptParser::ErrorCode::None;
            break;
#else
            error_ = ScriptParser::ErrorCode::SyntaxError;
            startIndex = index;
            return chain;
#endif
        case TokenType::FOR:
#if ET_ENABLE_SCRIPT_FOR
            chain.push_back(buildForLoopExpression(tokens, index));
            if (error())
            {
                startIndex = index; // Update the startIndex to the current index where the error occurred, so that the caller can know where the parsing stopped
                return chain;
            }
            error_ = ScriptParser::ErrorCode::None;
            break;
#else
            error_ = ScriptParser::ErrorCode::SyntaxError;
            startIndex = index;
            return chain;
#endif
        case TokenType::IF:
#if ET_ENABLE_SCRIPT_IF
            chain.push_back(buildIfExpression(tokens, index));
            if (error())
            {
                startIndex = index; // Update the startIndex to the current index where the error occurred, so that the caller can know where the parsing stopped
                return chain;
            }
            error_ = ScriptParser::ErrorCode::None;
            break;
#else
            error_ = ScriptParser::ErrorCode::SyntaxError;
            startIndex = index;
            return chain;
#endif
        case TokenType::FUNCTION:
#if ET_ENABLE_SCRIPT_FUNCTIONS
            chain.push_back(buildFunctionDefinitionExpression(tokens, index));
            if (error())
            {
                startIndex = index; // Update the startIndex to the current index where the error occurred, so that the caller can know where the parsing stopped
                return chain;
            }
            error_ = ScriptParser::ErrorCode::None;
            break;
#else
            error_ = ScriptParser::ErrorCode::SyntaxError;
            startIndex = index;
            return chain;
#endif
        default:
            break;
        }

        if (index >= tokens.size() || isStopToken(tokens[index].type, stopAt))
        {
            startIndex = index; // Update the startIndex to the current index where we encountered the stop token, so that the caller can know where the parsing stopped when we return
            return chain;
        }
    }
    startIndex = index; // If we successfully parsed until the end of the tokens or until we hit the stopAt token, we should update the startIndex to the end of the parsed tokens
    return chain;
}

Expression ScriptParser::buildFunctionCallExpression(const token_list_t &tokens, size_t &index)
{
    // Function call syntax: functionName(arg1 arg2 ...) <separators>
    // The arguments are optional and can optionally be separated by spaces or semicolons, but the parentheses are required.
    // The arguements can be WORD, VARIABLE, or even reserved keywords that are not logical operators or grouping (Parentheses/Curly/Square) tokens
    // As command name not possible are the reserved words
    if (index >= tokens.size() || tokens[index].type != TokenType::WORD)
    {
        error_ = ScriptParser::ErrorCode::UnexpectedToken;
        return Expression::createNil();
    }

    // Right after the function name there shouid ba the PAREN_OPEN token, otherwise it's a syntax error.
    if ((index + 1) >= tokens.size())
    {
        index = index + 1;
        error_ = ScriptParser::ErrorCode::UnexpectedEndOfInput;
        return Expression::createNil();
    }

    // Now find the PAREN_OPEN token
    size_t parenOpenIndex = index + 1;
    while (parenOpenIndex < tokens.size() && tokens[parenOpenIndex].type != TokenType::PAREN_OPEN)
    {
        ++parenOpenIndex;
    }

    size_t parenCloseIndex = index + 2; // Start looking for PAREN_CLOSE right after the PAREN_OPEN
    while (parenCloseIndex < tokens.size() && tokens[parenCloseIndex].type != TokenType::PAREN_CLOSE)
    {
        ++parenCloseIndex;
    }
    if (parenCloseIndex >= tokens.size())
    {
        index = parenCloseIndex; // Move index to the end of tokens since we have reached the end while looking for the closing parenthesis, so that the caller can know where the parsing stopped
        error_ = ScriptParser::ErrorCode::UnexpectedEndOfInput;
        return Expression::createNil();
    }

    const ETString functionName = tokens[index].text;
    token_list_t arguments;
    for (size_t argIndex = parenOpenIndex + 1; argIndex < parenCloseIndex; ++argIndex)
    {
        if (isSeparator(tokens[argIndex].type))
        {
            continue;
        }
        if (!isValidArgumentToken(tokens[argIndex].type))
        {
            error_ = ScriptParser::ErrorCode::InvalidFunctionArgument;
            return Expression::createNil();
        }
        arguments.push_back(tokens[argIndex]);
    }

    index = parenCloseIndex; // Move index to the closing parenthesis of the function call
    return Expression::createFunctionCall(TokenText(functionName), arguments);
}

Expression ScriptParser::buildCommandExpression(const token_list_t &tokens, size_t &index, const std::vector<TokenType> &stopAt)
{
    // Command syntax: commandName arg1 arg2 ... <separators>
    // The arguments are optional and can be separated by spaces , but the command name is required.
    // Optional redirections can be included in the arguments, e.g. "cmd arg1 arg2 > output.txt" or "cmd arg1 arg2 < input.txt" or "cmd arg1 arg2 >> append.txt". For simplicity, let's just say that redirection operators and their corresponding paths are treated as special arguments that are included in the command expression, and we will let the validation and execution stages handle whether they are valid or not.
    // The commandName have to be a valid keyword, the arguments can be anything but not logical operators (&&, ||, !)
    if (index >= tokens.size() || !isValidKeywordToken(tokens[index].type))
    {
        error_ = ScriptParser::ErrorCode::UnexpectedToken;
        return Expression::createNil();
    }

    // We terminate the command arguments when we encounter a separator token, an END_OF_FILE token, or any of the stopAt tokens. This allows us to correctly parse command expressions in various contexts, such as in pipelines (where the PIPE token is a stopAt token) or in if/while conditions (where the THEN and DO tokens are stopAt tokens).

    auto keyword = tokens[index];
    token_list_t argTokens;
    bool hasRedirectIn = false;
    bool hasRedirectOut = false;
    bool appendRedirect = false;
    Lang::token_t redirectInPath;
    Lang::token_t redirectOutPath;

    index++; // Move index to the first argument token (if any)
    while (index < tokens.size() && !isSeparator(tokens[index].type) && tokens[index].type != TokenType::END_OF_FILE && !isStopToken(tokens[index].type, stopAt))
    {
        switch (tokens[index].type)
        {
        case TokenType::REDIR_IN:
            hasRedirectIn = true;
            if ((index + 1) < tokens.size() && !isSeparator(tokens[index + 1].type) && tokens[index + 1].type != TokenType::END_OF_FILE && !isStopToken(tokens[index + 1].type, stopAt))
            {
                redirectInPath = tokens[index + 1];
                index += 2; // Skip the redirection operator and the path token
            }
            else
            {
                error_ = ScriptParser::ErrorCode::InvalidRedirection;
                index++; // Move index to the next token so that the caller can continue parsing from there after we return, even though we encountered an error with the current token. This allows the parser to skip over invalid tokens and attempt to parse the rest of the script, rather than getting stuck on the first error.
                return Expression::createNil();
            }
            break;
        case TokenType::REDIR_OUT:
        case TokenType::REDIR_APPEND:
            appendRedirect = (tokens[index].type == TokenType::REDIR_APPEND);
            hasRedirectOut = true;
            if ((index + 1) < tokens.size() && !isSeparator(tokens[index + 1].type) && tokens[index + 1].type != TokenType::END_OF_FILE && !isStopToken(tokens[index + 1].type, stopAt))
            {
                redirectOutPath = tokens[index + 1];
                index += 2; // Skip the redirection operator and the path token
            }
            else
            {
                error_ = ScriptParser::ErrorCode::InvalidRedirection;
                index++; // Move index to the next token so that the caller can continue parsing from there after we return, even though we encountered an error with the current token. This allows the parser to skip over invalid tokens and attempt to parse the rest of the script, rather than getting stuck on the first error.
                return Expression::createNil();
            }
            break;
        default:
            if (hasRedirectIn || hasRedirectOut) // If we have already encountered redirection operators, then any subsequent tokens that are not separators or stop tokens should be treated as invalid command arguments, since they cannot be part of the command arguments after redirection operators. This is a simplification, as in real shell syntax, redirection operators can appear anywhere in the command and can be interleaved with arguments, but for our simple scripting language, we will enforce that redirection operators must come after all command arguments for simplicity.
            {
                error_ = ScriptParser::ErrorCode::InvalidPipeline;
                return Expression::createNil();
            }
            if (!isValidCommandArgumentToken(tokens[index].type))
            {
                error_ = ScriptParser::ErrorCode::InvalidCommandArgument;
                index++; // Move index to the next token so that the caller can continue parsing from there after we return, even though we encountered an error with the current token. This allows the parser to skip over invalid tokens and attempt to parse the rest of the script, rather than getting stuck on the first error.
                return Expression::createNil();
            }
            argTokens.push_back(tokens[index]);
            index++;
            break;
        }
    }

    if (index > 0)
    {
        --index;
    }
    return Expression::createCommand(keyword, argTokens, hasRedirectIn, hasRedirectOut, appendRedirect, redirectInPath, redirectOutPath);
}

Expression ScriptParser::buildVariableAssignmentExpression(const token_list_t &tokens, size_t &index, const std::vector<TokenType> &stopAt)
{
    // Variable assignment syntax: variableName = value <separators>
    // The variableName should be a valid WORD token, the value can be anything but must be collapsable into a atomic value
    // (e.g. it cannot contain ungrouped command substitutions, pipelines, logical operators, or anything that would require more complex parsing to determine the value).
    // For simplicity, let's just say the value can be any sequence of tokens until a separator or a stop token is encountered, and we will let the validation and execution stages handle whether the value is valid or not. The equals sign is required to distinguish variable assignments from commands.
    if (index >= tokens.size() || tokens[index].type != TokenType::WORD)
    {
        // Should not happen since the caller should have already checked for the presence of the WORD token, but we check again just to be safe and avoid out-of-bounds access
        error_ = ScriptParser::ErrorCode::UnexpectedToken;
        return Expression::createNil();
    }
    token_t variableTargetToken = tokens[index];
    if ((index + 1) >= tokens.size() || tokens[index + 1].type != TokenType::EQUALS)
    { // Should not happen since the caller should have already checked for the presence of the EQUALS token, but we check again just to be safe and avoid out-of-bounds access
        error_ = ScriptParser::ErrorCode::UnexpectedToken;
        return Expression::createNil();
    }

    if ((index + 2) >= tokens.size())
    {
        index = index + 2; // Move index to the end of tokens since we have reached the end while looking for the value token, so that the caller can know where the parsing stopped
        error_ = ScriptParser::ErrorCode::UnexpectedEndOfInput;
        return Expression::createNil();
    }
    // Now we can have the following cases for the variable assignment value:
    // 1. A simple literal value (e.g. a WORD, a STRING_LITERAL, a VARIABLE token, etc.) that can be directly assigned to the variable without any further parsing or evaluation needed. In this case, we can just take the next token as the value of the variable assignment.
    // 2. A more complex expression that requires parsing to determine the value, such as a command substitution, a pipeline, a logical expression, etc. In this case, we can take all the tokens until the next separator or stop token as the value of the variable assignment, and let the validation and execution stages handle parsing and evaluating the value expression.

    auto currentToken = tokens[index + 2];
    switch (currentToken.type)
    {
    case TokenType::WORD:
    case TokenType::TRUE:
    case TokenType::FALSE:
    case TokenType::SINGLE_QUOTE_STRING_LITERAL:
    case TokenType::DOUBLE_QUOTE_STRING_LITERAL:
    case TokenType::VARIABLE:
        // Simple literal value case, we can just take the next token as the value of the variable assignment
        // But we have to check if the next token is a stopAt token or a separator, if not then it's an invalid command argument, since variable assignment cannot have complex expressions as values in this case
        if ((index + 3) < tokens.size() && !isStopToken(tokens[index + 3].type, stopAt) && !isSeparator(tokens[index + 3].type) && tokens[index + 3].type != TokenType::END_OF_FILE)
        {
            error_ = ScriptParser::ErrorCode::InvalidCommandArgument;
            return Expression::createNil();
        }
        index += 2; // Move index to the value token of the variable assignment
        return Expression::createVariableAssignment(variableTargetToken.text, Expression::createLiteral(tokens[index]));
    case TokenType::CURLY_OPEN:
    {
        size_t valueIndex = index + 2;
        Expression valueExpression = buildCurlyBracedPipelineExpression(tokens, valueIndex);
        if (error())
        {
            index = valueIndex;
            return Expression::createNil();
        }
        index = valueIndex;
        return Expression::createVariableAssignment(variableTargetToken.text, valueExpression);
    }
    default:
        break;
    }
    error_ = ScriptParser::ErrorCode::InvalidCommandArgument;
    return Expression::createNil();
}

Expression ScriptParser::buildCurlyBracedPipelineExpression(const token_list_t &tokens, size_t &index)
{
    // A curly-braced pipeline expression is a special type of command expression that represents a pipeline of commands enclosed in curly braces,
    // e.g. { cmd1 arg1 | cmd2 arg2 | cmd3 arg3 }
    // The entire pipeline expression is treated as a single value that can be assigned to a variable, or used in other expressions.

    if (index >= tokens.size() || tokens[index].type != TokenType::CURLY_OPEN)
    {
        error_ = ScriptParser::ErrorCode::UnexpectedToken;
        return Expression::createNil();
    }

    index++; // Move index to the first token after the opening curly brace

    auto pipeline = buildPipelineExpression(tokens, index, {TokenType::CURLY_CLOSE}); // We will look for the closing curly brace as the stop token for the pipeline expression, and we will let the buildPipelineExpression function handle parsing the commands in the pipeline and checking for syntax errors within the pipeline expression. If there are any syntax errors in the pipeline expression, buildPipelineExpression will set the error state accordingly and return a nil expression, which we will then return from this function as well.

    if (error())
    {
        return Expression::createNil();
    }

    if (index >= tokens.size() || tokens[index].type != TokenType::CURLY_CLOSE)
    {
        error_ = ScriptParser::ErrorCode::UnexpectedEndOfInput; // If we did not find a closing curly brace, then it's a syntax error due to unexpected end of input, since we were expecting a closing curly brace to match the opening curly brace at the beginning of the expression.
        return Expression::createNil();
    }

    return pipeline;
}

Expression ScriptParser::buildPipelineExpression(const token_list_t &tokens, size_t &index, const std::vector<TokenType> &stopAt)
{
    // A pipeline expression is a sequence of command expressions connected by PIPE tokens,
    // e.g. cmd1 arg1 | cmd2 arg2 | cmd3 arg3.
    ETVector<Expression> pipelineCommands;

    ETVector<TokenType> commandStopTokens = stopAt;
    commandStopTokens.push_back(TokenType::PIPE);

    size_t currentIndex = index;
    while (currentIndex < tokens.size())
    {
        if (isStopToken(tokens[currentIndex].type, stopAt) || tokens[currentIndex].type == TokenType::END_OF_FILE)
        {
            break;
        }

        Expression commandExpr = buildCommandExpression(tokens, currentIndex, commandStopTokens);
        if (error())
        {
            index = currentIndex;
            return Expression::createNil();
        }

        pipelineCommands.push_back(commandExpr);

        size_t delimiterIndex = currentIndex + 1;
        if (delimiterIndex >= tokens.size())
        {
            currentIndex = delimiterIndex;
            break;
        }

        if (tokens[delimiterIndex].type == TokenType::PIPE)
        {
            currentIndex = delimiterIndex + 1;
            if (currentIndex >= tokens.size())
            {
                index = delimiterIndex;
                error_ = ScriptParser::ErrorCode::InvalidPipeline;
                return Expression::createNil();
            }
            continue;
        }

        currentIndex = delimiterIndex;
        break;
    }

    if (pipelineCommands.empty())
    {
        error_ = ScriptParser::ErrorCode::InvalidPipeline;
        index = currentIndex;
        return Expression::createNil();
    }

    index = currentIndex;
    return Expression::createPipeline(pipelineCommands);
}

Expression ScriptParser::buildWhileLoopExpression(const token_list_t &tokens, size_t &index)
{
    // While loop syntax: while condition do ... done
    // The condition is a list of tokens between the 'while' keyword and the 'do' keyword, and the body is a list of expressions between the 'do' keyword and the matching 'done' keyword.
    if (index >= tokens.size() || tokens[index].type != TokenType::WHILE)
    {
        error_ = ScriptParser::ErrorCode::UnexpectedToken;
        return Expression::createNil();
    }

    // Condition starts right after the 'while' keyword and optional separators
    size_t condIndex = getNextMeaningfulTokenIndex(tokens, index + 1);
    // Looking for the keywords 'do' and 'done' to extract the condition and the body of the while loop
    size_t doIndex = index + 1;
    if (!findFirstIndexOfTokenType(tokens, TokenType::DO, doIndex))
    {
        error_ = ScriptParser::ErrorCode::UnexpectedEndOfInput;
        index = doIndex; // Move index to the end of the tokens since we have reached the end of input while looking for the 'do' keyword, so that the caller can know that we have reached the end of input when we return from this function
        return Expression::createNil();
    }

    auto condition = buildConditionExpression(tokens, condIndex, doIndex);
    if (error() || condIndex != doIndex || condition.tag != ExpressionType::Condition)
    {
        error_ = error() ? error_ : ScriptParser::ErrorCode::SyntaxError;
        index = condIndex; // Move index to the position where the error occurred or where we stopped parsing the condition, so that the caller can know where the parsing stopped when we return from this function
        return Expression::createNil();
    }

    // Now we need to parse the body of the while loop, which is the list of expressions between the 'do' keyword and the matching 'done' keyword
    size_t indexAfterDo = doIndex + 1;
    // The indexAfterDo is updated to the index of the 'done' keyword by parseExpressionChainUntilToken, so that after we return from building the while loop expression, the caller can continue parsing from the correct index after the entire while loop block
    ExpressionChain body = parseExpressionChainUntilToken(tokens, indexAfterDo, TokenType::DONE);
    if (error())
    {
        index = indexAfterDo; // Move index to the position where the error occurred, so that the caller can know where the parsing stopped when we return from this function
        return Expression::createNil();
    }

    if (expected(tokens, indexAfterDo, TokenType::DONE) != ScriptParser::ErrorCode::None)
    {
        error_ = expected(tokens, indexAfterDo, TokenType::DONE);
        index = indexAfterDo; // Move index to the position where we expected to find the 'done' token, so that the caller can continue parsing from there after we return from this function, even though we encountered an error with the current token. This allows the parser to skip over invalid tokens and attempt to parse the rest of the script, rather than getting stuck on the first error.
        return Expression::createNil();
    }

    if (body.empty())
    {
        error_ = ScriptParser::ErrorCode::EmptyWhileBody;
        index = indexAfterDo; // Move index to the 'done' keyword of the while loop, so that the caller can continue parsing from there even though we encountered an error with the empty body, rather than getting stuck on the first error.
        return Expression::createNil();
    }
    index = indexAfterDo - 1; // Move index to the 'done' keyword of the while loop, so that the caller can continue parsing from there after we return from this function. We move it to indexAfterDo-1 because parseExpressionChainUntilToken will move indexAfterDo to the position of the 'done' token when it encounters it, so we need to move it back by one to point to the 'done' token itself, since the caller will likely want to check for the 'done' token after we return to confirm that we have correctly parsed the while loop block.
    return Expression::createWhileLoop(condition, body);
}

Expression ScriptParser::buildForLoopExpression(const token_list_t &tokens, size_t &index)
{
    // For loop syntax: for variable = values do ... done
    // The variable is a single token that represents the loop variable, the values is a list of tokens between the 'in' keyword and the 'do' keyword, and the body is a list of expressions between the 'do' keyword and the matching 'done' keyword.
    if (index >= tokens.size() || tokens[index].type != TokenType::FOR)
    {
        error_ = ScriptParser::ErrorCode::UnexpectedToken;
        return Expression::createNil();
    }

    // The next token after 'for' should be the loop variable, which must be a WORD token
    size_t variableIndex = getNextMeaningfulTokenIndex(tokens, index + 1);

    if (variableIndex >= tokens.size())
    {
        index = variableIndex; // Move index to the end of tokens since we have reached the end of input while looking for the loop variable token, so that the caller can know that we have reached the end of input when we return from this function
        error_ = ScriptParser::ErrorCode::UnexpectedEndOfInput;
        return Expression::createNil();
    }

    if (expected(tokens, variableIndex, TokenType::WORD) != ScriptParser::ErrorCode::None)
    {
        error_ = expected(tokens, variableIndex, TokenType::WORD, ScriptParser::ErrorCode::InvalidForLoopVariable);
        index = variableIndex; // Move index to the position where we expected to find the loop variable token, so that the caller can continue parsing from there after we return, even though we encountered an error with the current token. This allows the parser to skip over invalid tokens and attempt to parse the rest of the script, rather than getting stuck on the first error.
        return Expression::createNil();
    }

    // After the loop variable, we should have an EQUALS token to assign the iteration values
    size_t equalsIndex = getNextMeaningfulTokenIndex(tokens, variableIndex + 1);

    if (expected(tokens, equalsIndex, TokenType::EQUALS) != ScriptParser::ErrorCode::None)
    {
        error_ = expected(tokens, equalsIndex, TokenType::EQUALS);
        index = equalsIndex; // Move index to the position where we expected to find the equals token, so that the caller can continue parsing from there after we return, even though we encountered an error with the current token. This allows the parser to skip over invalid tokens and attempt to parse the rest of the script, rather than getting stuck on the first error.
        return Expression::createNil();
    }

    // Looking for the keywords 'do' to extract the values
    size_t doIndex = getNextTokenIndex(tokens, TokenType::DO, equalsIndex + 1);

    if (expected(tokens, doIndex, TokenType::DO) != ScriptParser::ErrorCode::None)
    {
        error_ = expected(tokens, doIndex, TokenType::DO);
        index = doIndex; // Move index to the position where we expected to find the 'do' token, so that the caller can continue parsing from there after we return, even though we encountered an error with the current token. This allows the parser to skip over invalid tokens and attempt to parse the rest of the script, rather than getting stuck on the first error.
        return Expression::createNil();
    }

    TokenText variable(tokens[variableIndex].text);
    token_list_t values;
    for (size_t valueIndex = equalsIndex + 1; valueIndex < doIndex; ++valueIndex)
    {

        if (isSeparator(tokens[valueIndex].type))
        {
            continue;
        }
        if (!isValidForLoopValueToken(tokens[valueIndex].type))
        {
            error_ = ScriptParser::ErrorCode::InvalidForLoopValue;
            return Expression::createNil();
        }
        values.push_back(tokens[valueIndex]);
    }
    if (values.empty())
    {
        error_ = ScriptParser::ErrorCode::EmptyForLoopValues;
        return Expression::createNil();
    }
    // Now we need to parse the body of the for loop, which is the list of expressions between the 'do' keyword and the matching 'done' keyword
    // Out of construction, nested blocks are handled correctly in parseExpressionChainUntilToken, so we don't need to do extra work to handle nested for/while loops here - the caller will just continue parsing from the correct index after the entire for loop block
    size_t indexAfterDo = doIndex + 1;

    ExpressionChain body = parseExpressionChainUntilToken(tokens, indexAfterDo, TokenType::DONE);
    if (error())
    {
        return Expression::createNil();
    }
    if (indexAfterDo >= tokens.size() || tokens[indexAfterDo].type != TokenType::DONE)
    {
        error_ = ScriptParser::ErrorCode::UnexpectedEndOfInput; // If we did not find a 'done' token, then it's a syntax error due to unexpected end of input, since we were expecting a 'done' token to match the 'do' keyword at the beginning of the for loop block.
        index = indexAfterDo;
        return Expression::createNil();
    }

    if (body.empty())
    {
        error_ = ScriptParser::ErrorCode::EmptyForLoopBody;
        return Expression::createNil();
    }
    index = indexAfterDo; // Move index to the 'done' keyword of the for loop, so that the caller can continue parsing from there
    return Expression::createForLoop(variable, values, body);
}

Expression ScriptParser::buildIfExpression(const token_list_t &tokens, size_t &index)
{
    // If expression syntax: if condition then ... (elif condition then ...)* (else ...) fi
    // The condition is a list of tokens between the 'if' keyword and the 'then' keyword, the then-body is a list of expressions between the 'then' keyword and the next 'elif', 'else', or 'fi' keyword, the elif-conditions and elif-bodies are lists of tokens and lists of expressions between each 'elif' keyword and the next 'elif', 'else', or 'fi' keyword, and the else-body is a list of expressions between the 'else' keyword and the matching 'fi' keyword.

    // The returned Expression will be an IfExpression that contains:
    // - A list of conditions
    // - A list of then-bodies, where each then-body is a list of expressions corresponding to the condition at the same index in the conditions list. This allows us to support multiple 'elif' branches without needing to create nested IfExpressions for each 'elif' branch
    // - An optional else-body, which is a list of expressions that will be executed if none of the conditions are true

    ETVector<Expression> conditions;
    ETVector<ExpressionChain> thenBodies;
    ExpressionChain elseBody;

    if (expected(tokens, index, TokenType::IF) != ScriptParser::ErrorCode::None)
    {
        error_ = expected(tokens, index, TokenType::IF);
        return Expression::createNil();
    }

    size_t thenIndex = index + 1;
    if (!findFirstIndexOfTokenType(tokens, TokenType::THEN, thenIndex))
    {
        index = thenIndex; // Move index to the position where we expected to find the 'then' token, so that the caller can continue parsing from there after we return, even though we encountered an error with the current token. This allows the parser to skip over invalid tokens and attempt to parse the rest of the script, rather than getting stuck on the first error.
        error_ = ScriptParser::ErrorCode::UnexpectedEndOfInput;
        return Expression::createNil();
    }

    ETVector<TokenType> elifElseFiTokens = {TokenType::ELIF, TokenType::ELSE, TokenType::FI};

    size_t runningIndex = thenIndex + 1;

    size_t conditionStartIndex = index + 1;
    auto condition = buildConditionExpression(tokens, conditionStartIndex, thenIndex);
    if (conditionStartIndex != thenIndex || error() || condition.tag != ExpressionType::Condition)
    {
        // This means that buildConditionExpression did not consume all the tokens between 'if' and 'then', which indicates a syntax error in the condition expression
        index = conditionStartIndex; // Move index to the position where we expected to find the 'then' token, so that the caller can continue parsing from there after we return, even though we encountered an error with the current token. This allows the parser to skip over invalid tokens and attempt to parse the rest of the script, rather than getting stuck on the first error.
        error_ = error() ? error_ : ScriptParser::ErrorCode::SyntaxError;
        return Expression::createNil();
    }

    ExpressionChain body = parseExpressionChainUntilToken(tokens, runningIndex, elifElseFiTokens);
    if (error())
    {
        index = runningIndex; // Move index to the position where we expected to find the next 'elif', 'else', or 'fi' token, so that the caller can continue parsing from there after we return, even though we encountered an error with the current token. This allows the parser to skip over invalid tokens and attempt to parse the rest of the script, rather than getting stuck on the first error.
        return Expression::createNil();
    }

    conditions.push_back(condition);
    thenBodies.push_back(body);

    // Now parse every elif branch if there is any
    while (runningIndex < tokens.size() && (tokens[runningIndex].type == TokenType::ELIF))
    {
        size_t elifThenIndex = runningIndex + 1;
        if (!findFirstIndexOfTokenType(tokens, TokenType::THEN, elifThenIndex))
        {
            index = elifThenIndex; // Move index to the position where we expected to find the 'then' token, so that the caller can continue parsing from there after we return, even though we encountered an error with the current token. This allows the parser to skip over invalid tokens and attempt to parse the rest of the script, rather than getting stuck on the first error.
            error_ = ScriptParser::ErrorCode::UnexpectedEndOfInput;
            return Expression::createNil();
        }

        size_t elifConditionStartIndex = runningIndex + 1;
        Expression elifCondition = buildConditionExpression(tokens, elifConditionStartIndex, elifThenIndex);
        if (elifCondition.tag != ExpressionType::Condition || error() || elifConditionStartIndex != elifThenIndex)
        {
            error_ = error() ? error_ : ScriptParser::ErrorCode::SyntaxError;
            index = elifConditionStartIndex; // Move index to the position where we expected to find the 'then' token, so that the caller can continue parsing from there after we return, even though we encountered an error with the current token. This allows the parser to skip over invalid tokens and attempt to parse the rest of the script, rather than getting stuck on the first error.
            return Expression::createNil();
        }
        runningIndex = elifThenIndex + 1;
        ExpressionChain elifBody = parseExpressionChainUntilToken(tokens, runningIndex, elifElseFiTokens);
        if (error())
        {
            index = runningIndex; // Move index to the position where we expected to find the next 'elif', 'else', or 'fi' token, so that the caller can continue parsing from there after we return, even though we encountered an error with the current token. This allows the parser to skip over invalid tokens and attempt to parse the rest of the script, rather than getting stuck on the first error.
            return Expression::createNil();
        }
        conditions.push_back(elifCondition);
        thenBodies.push_back(elifBody);
    }

    // Now check if there is an else branch
    if (runningIndex < tokens.size() && tokens[runningIndex].type == TokenType::ELSE)
    {
        runningIndex++;
        elseBody = parseExpressionChainUntilToken(tokens, runningIndex, TokenType::FI);
        if (error())
        {
            index = runningIndex; // Move index to the position where we expected to find the 'fi' token, so that the caller can continue parsing from there after we return, even though we encountered an error with the current token. This allows the parser to skip over invalid tokens and attempt to parse the rest of the script, rather than getting stuck on the first error.
            return Expression::createNil();
        }
    }

    // Now we should be at the 'fi' token that ends the entire if expression
    if (expected(tokens, runningIndex, TokenType::FI) != ScriptParser::ErrorCode::None)
    {
        error_ = expected(tokens, runningIndex, TokenType::FI);
        index = runningIndex; // Move index to the position where we expected to find the 'fi' token, so that the caller can continue parsing from there after we return, even though we encountered an error with the current token. This allows the parser to skip over invalid tokens and attempt to parse the rest of the script, rather than getting stuck on the first error.
        return Expression::createNil();
    }
    index = runningIndex; // Move index to the 'fi' token of the if expression, so that the caller can continue parsing from there
    error_ = ScriptParser::ErrorCode::None;
    return Expression::createIfExpression(conditions, thenBodies, elseBody);
}

Expression ScriptParser::buildFunctionDefinitionExpression(const token_list_t &tokens, size_t &index)
{
    // Function definition syntax: function functionName(arg1 arg2 ...) ... done
    // The functionName is a single token that represents the name of the function,
    // the arguments are a list of WORD tokens between the following PARENTHESES
    // the body is a list of expressions between the closing parentheses and the matching 'done' keyword.
    if (index >= tokens.size() || tokens[index].type != TokenType::FUNCTION)
    {
        error_ = ScriptParser::ErrorCode::UnexpectedToken;
        return Expression::createNil();
    }
    // The next token after 'function' should be the function name, which must be a WORD token
    size_t functionNameIndex = getNextMeaningfulTokenIndex(tokens, index + 1); // Move functionNameIndex to the next non-separator token, which should be the function name token. If we encounter any separators, we will skip over them until we find a non-separator token or reach the end of the tokens.

    if (expected(tokens, functionNameIndex, TokenType::WORD) != ScriptParser::ErrorCode::None)
    {
        error_ = expected(tokens, functionNameIndex, TokenType::WORD, ScriptParser::ErrorCode::SyntaxError);
        return Expression::createNil();
    }

    // Right after the function name there should be the PAREN_OPEN token, otherwise it's a syntax error.
    size_t parenOpenIndex = getNextMeaningfulTokenIndex(tokens, functionNameIndex + 1); // Move parenOpenIndex to the next non-separator token, which should be the opening parenthesis token. If we encounter any separators, we will skip over them until we find a non-separator token or reach the end of the tokens.
    if (expected(tokens, parenOpenIndex, TokenType::PAREN_OPEN) != ScriptParser::ErrorCode::None)
    {
        error_ = expected(tokens, parenOpenIndex, TokenType::PAREN_OPEN, ScriptParser::ErrorCode::SyntaxError);
        return Expression::createNil();
    }

    size_t parenCloseIndex = parenOpenIndex + 1;

    if (!findFirstIndexOfTokenType(tokens, TokenType::PAREN_CLOSE, parenCloseIndex))
    {
        error_ = ScriptParser::ErrorCode::UnexpectedEndOfInput;
        return Expression::createNil();
    }

    const ETString functionName = tokens[functionNameIndex].text;
    ETVector<TokenText> arguments;
    for (size_t argIndex = parenOpenIndex + 1; argIndex < parenCloseIndex; ++argIndex)
    {
        if (isSeparator(tokens[argIndex].type))
        {
            continue;
        }
        if (tokens[argIndex].type != TokenType::WORD)
        {
            error_ = ScriptParser::ErrorCode::InvalidFunctionArgument;
            return Expression::createNil();
        }
        arguments.push_back(tokens[argIndex].text);
    }

    size_t runningIndex = parenCloseIndex + 1;
    ExpressionChain body = parseExpressionChainUntilToken(tokens, runningIndex, TokenType::DONE);
    if (error())
    {
        return Expression::createNil();
    }

    if (expected(tokens, runningIndex, TokenType::DONE) != ScriptParser::ErrorCode::None)
    {
        error_ = expected(tokens, runningIndex, TokenType::DONE);
        index = runningIndex; // Move index to the position where we expected to find the 'done' token, so that the caller can continue parsing from there after we return, even though we encountered an error with the current token. This allows the parser to skip over invalid tokens and attempt to parse the rest of the script, rather than getting stuck on the first error.
        return Expression::createNil();
    }
    if (body.empty())
    {
        error_ = ScriptParser::ErrorCode::EmptyFunctionBody;
        return Expression::createNil();
    }

    // Also check if the function definitions ends with a 'done' keyword, if not it's a syntax error

    index = runningIndex + 1; // Move index to the next token after the 'done' keyword, so that the caller can continue parsing from there
    error_ = ScriptParser::ErrorCode::None;
    return Expression::createFunctionDefinition(TokenText(functionName), arguments, body);
}

Expression ScriptParser::buildConditionExpression(const token_list_t &tokens, size_t &index, size_t endIndex)
{
    // We can filter out any separator tokens from the condition tokens, since they are not meaningful in conditions and can be safely ignored during parsing and execution of conditions
    // Also we build the ConditionExpression as combination of nodes and connecting operators.
    // A node can be:
    // - A boolean literal (true or false)
    // - A variable (e.g. $var)
    // - A grouped condition enclosed in parentheses (e.g. (condition tokens))
    // - A function call (e.g. funcName(arg1 arg2))
    // - A string literal (e.g. "string" or 'string')
    // - A word token (e.g. WORD), which can be treated as a string literal in conditions
    // - A comparison expression (e.g. [ arg1 == arg2 ], [arg1 != arg2] )
    // - But a node cannot be a logical operator (&&, ||) token, since logical operators are used to connect nodes in a condition, and they cannot be treated as standalone nodes in a condition. So if we encounter a logical operator token in the condition tokens, then it's not a valid node by itself, and we should treat it as a connecting operator that connects the previous node and the next node in the condition.
    // The connecting operators can only be logical AND (&&) or logical OR (||),

    ETVector<ETPair<Expression, bool>> conditionNodes;
    ETVector<TokenType> connectingOperators; // This will store the logical operators (&&, ||) that connect the condition nodes, in the order they appear in the condition tokens. The number of connecting operators should always be one less than the number of condition nodes, since each connecting operator connects two condition nodes.

    bool negateNextNode = false; // This flag indicates whether the next condition node should be negated by a NOT operator. When we encounter a NOT token, we will set this flag to true, and then when we create the next condition node, we will store it as a pair of (conditionNodeExpression, isNegated) in the conditionNodes vector, where isNegated is the value of negateNextNode at that time. After creating the condition node, we will reset negateNextNode to false for the next node.
    TokenType lastToken = TokenType::NIL;
    bool terminatedByCloseParen = false;
    for (size_t i = index; i < endIndex && !terminatedByCloseParen && tokens[i].type != TokenType::END_OF_FILE; ++i)
    {
        if (isSeparator(tokens[i].type))
        {
            continue;
        }

        // Guard rails for syntax errors in condition expressions:
        // 1. Two logical operators cannot be adjacent to each other, and a logical operator cannot be the first token in the condition, so if we encounter a logical operator token right after another logical operator token or at the beginning of the condition tokens, then it's a syntax error.
        // 2. A NOT must follow a logical operator or another NOT, or be the first token in the condition, so if we encounter a NOT token that does not follow a logical operator or another NOT token, and is not the first token in the condition tokens, then it's a syntax error.
        // 3. Two nodes cannot be adjacent to each other without a logical operator in between, so if we encounter a node token right after another node token, then it's a syntax error.
        if (isBinaryOperatorToken(tokens[i].type) && (isBinaryOperatorToken(lastToken) || lastToken == TokenType::NIL))
        {
            // A binary operator token (e.g. ==, !=, <, >, etc.) cannot be the first token in the condition, and can not follow an other binary operator token.
            index = i; // Move index to the position of the unexpected logical operator token, so that the caller can report the syntax error at the correct token
            error_ = ScriptParser::ErrorCode::SyntaxError;
            return Expression::createNil();
        }
        else if (!isBinaryOperatorToken(tokens[i].type) && !isLogicalOperatorToken(tokens[i].type) && tokens[i].type != TokenType::PAREN_CLOSE && tokens[i].type != TokenType::CURLY_CLOSE && tokens[i].type != TokenType::SQUARE_CLOSE && (!isLogicalOperatorToken(lastToken) && lastToken != TokenType::NIL))
        {
            // A node token must follow a logical operator token or be the first token in the condition, so if we encounter a node token that does not follow a logical operator token and is not the first token in the condition tokens, then it's a syntax error.
            index = i; // Move index to the position of the unexpected node token, so that the caller can report the syntax error at the correct token
            error_ = ScriptParser::ErrorCode::SyntaxError;
            return Expression::createNil();
        }

        switch (tokens[i].type)
        {
        case TokenType::PAREN_CLOSE:
            // A closing parenthesis indicate the end of the current grouped condition. So we return the current ConditionExpression that we have built so far,
            // and let the caller handle the rest of the tokens after the closing parenthesis.
            index = i; // Move index to the closing parenthesis token, so that the caller can continue parsing from there
            terminatedByCloseParen = true;
            break;
        case TokenType::AND_AND:
        case TokenType::OR_OR:
            connectingOperators.push_back(tokens[i].type);
            break;
        case TokenType::NOT:
            // A NOT token revert the boolean value of the next node in the condition
            negateNextNode = !negateNextNode; // Toggle the negateNextNode flag when we encounter a NOT token, since multiple NOT tokens can cancel each other out (e.g. !!condition is equivalent to condition)
            break;
        case TokenType::TRUE:
        case TokenType::FALSE:
        case TokenType::SINGLE_QUOTE_STRING_LITERAL:
        case TokenType::DOUBLE_QUOTE_STRING_LITERAL:
        case TokenType::VARIABLE:
            // These tokens can be treated as literal nodes in the condition expression, so we create a condition node for them and add it to the conditionNodes vector, along with the negateNextNode flag to indicate whether this node should be negated by a NOT operator
            conditionNodes.push_back({Expression::createLiteral(tokens[i]), negateNextNode});
            negateNextNode = false; // Reset negateNextNode after creating a condition node, since the NOT operator only applies to the next node immediately following it
            break;
        case TokenType::PAREN_OPEN:
            // A grouped condition enclosed in parentheses can be treated as a single node in the condition expression, it is by itself a ConditionExpression that needs to be parsed recursively from the tokens between the parentheses, and then we can create a condition node for it and add it to the conditionNodes vector, along with the negateNextNode flag to indicate whether this node should be negated by a NOT operator
            i = i + 1; // Move i to the first token inside the parentheses
            conditionNodes.push_back({buildConditionExpression(tokens, i, endIndex), negateNextNode});
            if (error())
            {
                index = i; // Move index to the position of the token that caused the error in the grouped condition, so that the caller can report the syntax error at the correct token
                return Expression::createNil();
            }
            negateNextNode = false; // Reset negateNextNode after creating a condition node, since the NOT operator only applies to the next node immediately following it
            // Also we have to check if the last token was a PAREN_CLOSE, since we have to check if there was a valid condition node
            if (tokens[i].type != TokenType::PAREN_CLOSE)
            {
                // If after parsing the grouped condition we do not find a closing parenthesis, then it's a syntax error.
                error_ = ScriptParser::ErrorCode::SyntaxError;
                index = i; // Move index to the position of the unexpected token after the grouped condition, so that the caller can report the syntax error at the correct token
                return Expression::createNil();
            }
            break;
        case TokenType::CURLY_OPEN:
            // Command substitutions are not allowed in conditions.
            // If commands are just executed in variable assignment or as atomic expressions the runner is much simpler
            index = i; // Move index to the position of the unexpected token after the command substitution, so that the caller can report the syntax error at the correct token
            error_ = ScriptParser::ErrorCode::SyntaxError;
            return Expression::createNil();
            break;
        case TokenType::SQUARE_OPEN:
            // A comparison expression enclosed in square brackets can be treated as a single node in the condition expression, it is by itself a ComparisonExpression that needs to be parsed from the tokens between the square brackets, and then we can create a condition node for it and add it to the conditionNodes vector, along with the negateNextNode flag to indicate whether this node should be negated by a NOT operator
            conditionNodes.push_back({buildComparisonExpression(tokens, i), negateNextNode});
            if (error())
            {
                index = i; // Move index to the position of the token that caused the error in the comparison expression, so that the caller can report the syntax error at the correct token
                return Expression::createNil();
            }
            negateNextNode = false; // Reset negateNextNode after creating a condition node, since the NOT operator only applies to the next node immediately following it
            if (tokens[i].type != TokenType::SQUARE_CLOSE)
            {
                // If after parsing the comparison expression we do not find a closing square bracket, then it's a syntax error.
                index = i; // Move index to the position of the unexpected token after the comparison expression, so that the caller can report the syntax error at the correct token
                error_ = ScriptParser::ErrorCode::SyntaxError;
                return Expression::createNil();
            }
            break;
        default:
            // If we encounter any other token type in the condition tokens, then it's a syntax error, since only the above token types are valid in condition expressions.
            error_ = ScriptParser::ErrorCode::SyntaxError;
            index = i; // Move index to the position of the unexpected token, so that the caller can report the syntax error at the correct token
            return Expression::createNil();
        }
        lastToken = tokens[i].type;
    }
    if (lastToken == TokenType::NIL)
    {
        // If we have not found any non-separator tokens in the condition tokens, then it's an empty condition, which is a syntax error.
        error_ = ScriptParser::ErrorCode::EmptyCondition;
        index = endIndex; // Move index to the endIndex, so that the caller can report the syntax error at the correct token (which is the token right after the empty condition)
        return Expression::createNil();
    }

    if (lastToken == TokenType::AND_AND || lastToken == TokenType::OR_OR)
    {
        // A logical operator cannot be the last token in the condition, so if we encounter a logical operator token at the end of the condition tokens, then it's a syntax error.
        error_ = ScriptParser::ErrorCode::SyntaxError;
        index = endIndex; // Move index to the endIndex, so that the caller can report the syntax error at the correct token (which is the token right after the logical operator)
        return Expression::createNil();
    }

    if (!terminatedByCloseParen)
    {
        index = endIndex;
    }
    return Expression::createConditionExpression(conditionNodes, connectingOperators);
}

Expression ScriptParser::buildComparisonExpression(const token_list_t &tokens, size_t &index)
{
    // Syntax "[<expression> <comparison_operator> <expression>]", where the comparison operators can be: ==, !=, (<, >, <=, >=)

    if (index >= tokens.size() || tokens[index].type != TokenType::SQUARE_OPEN)
    {
        error_ = ScriptParser::ErrorCode::UnexpectedToken;
        return Expression::createNil();
    }

    auto parseOperand = [&](size_t &operandIndex, Expression &operandExpression) -> bool
    {
        if (operandIndex >= tokens.size())
        {
            index = operandIndex; // Move index to the position where we expected to find the operand token, so that the caller can report the syntax error at the correct token
            error_ = ScriptParser::ErrorCode::UnexpectedEndOfInput;
            return false;
        }

        switch (tokens[operandIndex].type)
        {
        case TokenType::TRUE:
        case TokenType::FALSE:
        case TokenType::SINGLE_QUOTE_STRING_LITERAL:
        case TokenType::DOUBLE_QUOTE_STRING_LITERAL:
        case TokenType::VARIABLE:
            operandExpression = Expression::createLiteral(tokens[operandIndex]);
            return true;
        default:
            error_ = ScriptParser::ErrorCode::SyntaxError;
            return false;
        }
    };

    size_t currentIndex = index + 1;
    if (currentIndex >= tokens.size())
    {
        index = currentIndex; // Move index to the position where we expected to find the first operand token, so that the caller can report the syntax error at the correct token
        error_ = ScriptParser::ErrorCode::UnexpectedEndOfInput;
        return Expression::createNil();
    }

    Expression leftExpression;
    if (!parseOperand(currentIndex, leftExpression))
    {
        index = currentIndex; // Move index to the position where we expected to find the first operand token, so that the caller can report the syntax error at the correct token
        if (!error())
        {
            error_ = ScriptParser::ErrorCode::SyntaxError;
        }
        return Expression::createNil();
    }

    ++currentIndex;
    if (currentIndex >= tokens.size())
    {
        index = currentIndex; // Move index to the position where we expected to find the comparison operator token, so that the caller can report the syntax error at the correct token
        error_ = ScriptParser::ErrorCode::UnexpectedEndOfInput;
        return Expression::createNil();
    }

    if (!isComparisonOperatorToken(tokens[currentIndex].type))
    {
        index = currentIndex; // Move index to the position where we expected to find the comparison
        error_ = ScriptParser::ErrorCode::SyntaxError;
        return Expression::createNil();
    }

    TokenType comparisonOperator = tokens[currentIndex].type;

    ++currentIndex;
    if (currentIndex >= tokens.size())
    {
        index = currentIndex; // Move index to the position where we expected to find the second operand token, so that the caller can report the syntax error at the correct token
        error_ = ScriptParser::ErrorCode::UnexpectedEndOfInput;
        return Expression::createNil();
    }

    Expression rightExpression;
    if (!parseOperand(currentIndex, rightExpression))
    {
        index = currentIndex; // Move index to the position where we expected to find the second operand token, so that the caller can report the syntax error at the correct token
        if (!error())
        {
            error_ = ScriptParser::ErrorCode::SyntaxError;
        }
        return Expression::createNil();
    }

    if ((currentIndex + 1) >= tokens.size() || tokens[currentIndex + 1].type != TokenType::SQUARE_CLOSE)
    {
        error_ = ScriptParser::ErrorCode::SyntaxError;
        return Expression::createNil();
    }

    index = currentIndex + 1; // Leave the caller on the closing square bracket.
    return Expression::createComparison(leftExpression, comparisonOperator, rightExpression);
}

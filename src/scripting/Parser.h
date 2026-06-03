#pragma once
#include "lang/LangAPI.h"

namespace EmbeddedTerminal
{
    using namespace Lang;
    namespace Scripting
    {

        enum class ExpressionType
        {
            Command,
            If,
            WhileLoop,
            ForLoop,
            FunctionDefinition,
            FunctionCall,
            VariableAssignment,
            Pipeline,
            Condition,
            Literal,
            Comparison,
            Nil,
        };

        // AST node types - plain data structures
        struct CommandExpression
        {
            token_t keyword;
            token_list_t arguments;
            bool hasRedirectIn;
            bool hasRedirectOut;
            bool appendRedirect;
            token_t redirectInPath;
            token_t redirectOutPath;
        };

        struct Expression;
        typedef ETVector<Expression> ExpressionChain;

        struct ConditionExpression
        {
            ETVector<ETPair<Expression, bool>> conditionNodes; // The list of condition nodes in the condition, in the order they appear in the condition tokens. Each node is represented as a pair of (conditionNodeExpression, isNegated), where conditionNodeExpression is an expression that represents the condition node (e.g. a command expression that should be executed and evaluated for its exit code), and isNegated is a boolean flag that indicates whether the condition node is negated by a NOT operator. This allows us to correctly represent conditions with multiple NOT operators applied to different nodes in the condition.
            ETVector<TokenType> connectingOperators;           // The list of logical operators (&&,

            ConditionExpression() = default;
            ConditionExpression(const ETVector<ETPair<Expression, bool>> &nodes, const ETVector<TokenType> &operators)
                : conditionNodes(nodes), connectingOperators(operators) {}
        };

        struct LiteralExpression
        {
            token_t literalToken; // The token representing the literal value, e.g. a WORD, a STRING_LITERAL, a VARIABLE token, etc.
        };

        struct ComparisonExpression
        {
            std::unique_ptr<Expression> leftOperand;  // Use a pointer to avoid infinite recursion in the type definition, since Expression contains a unique_ptr to ComparisonExpression. The left operand can be empty for cases like "== VAR2" where the left operand is missing, which is a syntax error that we can detect during parsing.
            TokenType operatorType;                   // The comparison operator, e.g. EQUALS_EQUALS (==) or NOT_EQUALS (!=)
            std::unique_ptr<Expression> rightOperand; // Use a pointer to avoid infinite recursion in the type definition, since Expression contains a unique_ptr to ComparisonExpression. The right operand can be empty for cases like "VAR1 ==" where the right operand is missing, which is a syntax error that we can detect during parsing.
        };

        struct IfExpression
        {
            ETVector<Expression> conditions;  // The conditions for the if and elif branches. The first condition corresponds to the if branch, and the subsequent conditions correspond to the elif branches in order. The else branch is represented by an empty condition.
            ETVector<ExpressionChain> bodies; // The bodies for the if and elif branches. Each body is a list of expressions corresponding to the condition at the same index in the conditions list. The else branch is represented by an empty body.
            ETVector<Expression> elseBody;
        };

        struct WhileLoopExpression
        {
            std::unique_ptr<Expression> condition;
            ETVector<Expression> body;
        };

        struct ForLoopExpression
        {
            // for i = 1 2 3 do body done
            TokenText variable;
            token_list_t values; // The iteration values
            ETVector<Expression> body;
        };

        struct NilExpression
        {
        };

        struct FunctionDefinitionExpression
        {
            TokenText functionName;
            ETVector<TokenText> parameters;
            ETVector<Expression> body;
        };

        struct FunctionCallExpression
        {
            TokenText functionName;
            token_list_t arguments;
        };

        struct VariableAssignmentExpression
        {
            TokenText variable;
            std::unique_ptr<Expression> valueExpression; // Have to be a pointer since Expression contains a unique_ptr to VariableAssignmentExpression, so we can't have it directly as a member without causing infinite recursion in the type definition. Using a unique_ptr also allows us to have an empty valueExpression for cases like "var=" where the variable is assigned an empty value.

            VariableAssignmentExpression() = default;
            VariableAssignmentExpression(const TokenText &variable, const std::unique_ptr<Expression> &valueExpression) : variable(variable), valueExpression(valueExpression ? std::make_unique<Expression>(*valueExpression) : nullptr) {}
        };

        struct PipelineExpression
        {
            ETVector<Expression> commands; // The list of command expressions in the pipeline, in the order they appear in the pipeline. Each command expression can be a simple command or a more complex expression like a function call or variable assignment, as long as it can be executed as a command.
        };

        // Tagged union AST node with function pointer table
        // Enables recursive execution for nested scripts (e.g., "for i = 1 2 3 do script ./echo.et i done")
        struct Expression
        {
            union Storage
            {
                CommandExpression command;
                IfExpression ifExpr;
                WhileLoopExpression whileLoop;
                ForLoopExpression forLoop;
                NilExpression nil;
                FunctionDefinitionExpression functionDef;
                FunctionCallExpression functionCall;
                VariableAssignmentExpression variableAssignment;
                PipelineExpression pipeline;     // A special type of command expression that represents a pipeline of commands, e.g. { cmd1 arg1 | cmd2 arg2 | cmd3 arg3 }
                ConditionExpression condition;   // A special type of expression that represents a condition expression, which is a combination of condition nodes and logical operators. This allows us to represent complex conditions with multiple nodes and logical operators in a single expression, which can be evaluated directly during execution without needing to construct separate expressions for each node and operator.
                LiteralExpression literal;       // A special type of command expression that represents a literal value, e.g. a WORD, a STRING_LITERAL, a VARIABLE token, etc. The keyword field will store the literal token, and the arguments field will be empty.
                ComparisonExpression comparison; // A special type of expression that represents a comparison expression, which is a combination of two operands and a comparison operator. This allows us to represent comparisons like "VAR1 == VAR2" or "VAR1 != VAR2" as a single expression that can be evaluated directly during execution.
                Storage() {}
                ~Storage() {}
            } data;

            ExpressionType tag;

            // Constructor/Destructor
            Expression();
            ~Expression();
            Expression(const Expression &other);
            Expression &operator=(const Expression &other);

            void destroy();
            void copyFrom(const Expression &other);

            // Factory methods for creating expressions
            static Expression createCommand(const token_t &keyword, const token_list_t &arguments, bool hasRedirectIn = false, bool hasRedirectOut = false, bool appendRedirect = false, const token_t &redirectInPath = token_t(), const token_t &redirectOutPath = token_t());
            static Expression createIfExpression(
                const ETVector<Expression> &conditions,
                const ETVector<ExpressionChain> &bodies,
                const ETVector<Expression> &elseBody);
            static Expression createWhileLoop(const Expression &cond,
                                              const ETVector<Expression> &body);
            static Expression createForLoop(const TokenText &variable,
                                            const token_list_t &values,
                                            const ETVector<Expression> &body);
            static Expression createFunctionDefinition(const TokenText &functionName,
                                                       const ETVector<TokenText> &parameters,
                                                       const ETVector<Expression> &body);
            static Expression createFunctionCall(const TokenText &functionName, const token_list_t &arguments);
            static Expression createVariableAssignment(const TokenText &variable, const Expression &valueExpression);
            static Expression createPipeline(const ETVector<Expression> &commands);
            static Expression createNil();
            static Expression createConditionExpression(const ETVector<ETPair<Expression, bool>> &conditionNodes, const ETVector<TokenType> &connectingOperators);
            static Expression createLiteral(const token_t &literalToken);
            static Expression createComparison(const Expression &leftOperand, TokenType operatorType, const Expression &rightOperand);
        };

        class ScriptParser
        {
        public:
            enum class ErrorCode
            {
                None = 0,
                EmptyInput,
                SyntaxError,
                UnexpectedEndOfInput,
                InvalidFunctionArgument,
                InvalidCommandArgument,
                EmptyCondition,
                EmptyWhileBody,
                UnexpectedToken,
                InvalidForLoopVariable,
                InvalidForLoopValue,
                EmptyForLoopValues,
                EmptyForLoopBody,
                MissingEqualsInForLoop,
                EmptyFunctionBody,
                InvalidPipeline,
                InvalidRedirection,
            };

            bool error() const
            {
                return error_ != ScriptParser::ErrorCode::None;
            }

            ScriptParser::ErrorCode getError() const
            {
                return error_;
            }

            ExpressionChain parse(const token_list_t &tokens);
            ExpressionChain parse(const token_list_t &tokens, size_t &startIndex);

        protected:
            /// @brief Parses a chain of expressions until a specified token is encountered
            /// @param tokens The list of tokens to parse from
            /// @param startIndex  The index in the tokens list to start parsing from.
            /// This will be updated to the index of the stop token or separator or end of file that caused the parsing to stop, so that the caller can know where the parsing stopped when this function returns.
            /// @param stopAt The token type that indicates where to stop parsing the expression chain. This can be a single token type or a list of token types, depending on which overload of the function is used. The parsing will stop when any of the specified token types is encountered in the tokens list.
            /// @return The chain of expressions parsed from the tokens list until the stop token is encountered. If a syntax error is encountered during parsing, the error state will be set accordingly and an empty expression chain will be returned.
            ExpressionChain parseExpressionChainUntilToken(const token_list_t &tokens, size_t &startIndex, TokenType stopAt);
            /// @brief Parses a chain of expressions until any of the specified token types is encountered
            /// @param tokens The list of tokens to parse from
            /// @param startIndex  The index in the tokens list to start parsing from. This will be updated to the index of the stop token or separator or end of file that caused the parsing to stop, so that the caller can know where the parsing stopped when this function returns.
            /// @param stopAt The list of token types that indicate where to stop parsing the expression chain. The parsing will stop when any of the specified token types is encountered in the tokens list.
            /// @return The chain of expressions parsed from the tokens list until any of the stop tokens is encountered. If a syntax error is encountered during parsing, the error state will be set accordingly and an empty expression chain will be returned.
            ExpressionChain parseExpressionChainUntilToken(const token_list_t &tokens, size_t &startIndex, const ETVector<TokenType> &stopAt);
            /// @brief Parses a function call expression from the tokens list starting at the specified index. The function call expression is expected to be in the form of "functionName arg1 arg2 ...", where functionName is a WORD token representing the name of the function being called, and arg1, arg2, etc. are the arguments to the function, which can be WORD tokens, STRING_LITERAL tokens, VARIABLE tokens, or more complex expressions like pipelines or command substitutions.
            /// @param tokens The list of tokens to parse from
            /// @param index The index in the tokens list to start parsing from. This will be updated to the index of the last token that is part of the function call expression, so that the caller can know where the parsing of the function call expression ended when this function returns.
            /// @return The parsed function call expression, or an empty expression if an error is encountered. If a syntax error is encountered during parsing, the error state will be set accordingly and an empty expression will be returned.
            Expression buildFunctionCallExpression(const token_list_t &tokens, size_t &index);
            Expression buildCommandExpression(const token_list_t &tokens, size_t &index, const std::vector<TokenType> &stopAt);
            Expression buildVariableAssignmentExpression(const token_list_t &tokens, size_t &index, const std::vector<TokenType> &stopAt);
            Expression buildCurlyBracedPipelineExpression(const token_list_t &tokens, size_t &index);
            /// @brief Parses a pipeline expression from the tokens list starting at the specified index. The pipeline expression is expected to be in the form of "cmd1 arg1 | cmd2 arg2 | cmd3 arg3", where cmd1, cmd2, cmd3 are command keywords (WORD tokens) representing the commands in the pipeline, and arg1, arg2, arg3 are the arguments to each command, which can be WORD tokens, STRING_LITERAL tokens, VARIABLE tokens, or more complex expressions like nested pipelines or command substitutions. The PIPE token is used to separate the commands in the pipeline.
            /// @param tokens The list of tokens to parse from
            /// @param index The index in the tokens list to start parsing from. This will be updated to the index of the last token that is part of the pipeline expression, so that the caller can know where the parsing of the pipeline expression ended when this function returns.
            /// @param stopAt The list of token types that indicate where to stop parsing the pipeline expression. The parsing will stop when any of the specified token types is encountered in the tokens list.
            /// @return The parsed pipeline expression, or an empty expression if an error is encountered. If a syntax error is encountered during parsing, the error state will be set accordingly and an empty expression will be returned.
            Expression buildPipelineExpression(const token_list_t &tokens, size_t &index, const std::vector<TokenType> &stopAt);
            Expression buildWhileLoopExpression(const token_list_t &tokens, size_t &index);
            Expression buildForLoopExpression(const token_list_t &tokens, size_t &index);
            Expression buildIfExpression(const token_list_t &tokens, size_t &index);
            Expression buildFunctionDefinitionExpression(const token_list_t &tokens, size_t &index);
            Expression buildConditionExpression(const token_list_t &tokens, size_t &index, size_t endIndex);
            Expression buildComparisonExpression(const token_list_t &tokens, size_t &index);

        protected:
            ScriptParser::ErrorCode error_ = ScriptParser::ErrorCode::None;
        };
    }
}
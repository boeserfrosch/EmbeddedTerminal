#include "TokenUtils.h"

namespace EmbeddedTerminal
{
    namespace Lang
    {
        bool isReservedWordToken(const TokenType type)
        {
            return (type == TokenType::WHILE || type == TokenType::FOR || type == TokenType::IF || type == TokenType::THEN ||
                    type == TokenType::ELSE || type == TokenType::ELIF || type == TokenType::FI || type == TokenType::DO ||
                    type == TokenType::DONE || type == TokenType::FUNCTION);
        }

        bool isComparisonOperatorToken(const TokenType type)
        {
            return type == TokenType::EQUALS_EQUALS || type == TokenType::NOT_EQUALS;
        }

        bool isLogicalOperatorToken(const TokenType type)
        {
            return type == TokenType::AND_AND || type == TokenType::OR_OR || type == TokenType::NOT;
        }

        bool isBinaryOperatorToken(const TokenType type)
        {
            return isComparisonOperatorToken(type) || type == TokenType::AND_AND || type == TokenType::OR_OR;
        }

        bool isUnaryOperatorToken(const TokenType type)
        {
            return type == TokenType::NOT;
        }

        bool isSeparator(const TokenType type)
        {
            return type == TokenType::SEMI || type == TokenType::NEWLINE;
        }

        bool isStopToken(const TokenType type, const std::vector<TokenType> &stopAt)
        {
            for (const TokenType stopToken : stopAt)
            {
                if (type == stopToken)
                {
                    return true;
                }
            }
            return false;
        }

        bool isShellOperatorToken(const token_t &token)
        {
            return token.type == TokenType::PIPE || token.type == TokenType::REDIR_OUT || token.type == TokenType::REDIR_IN ||
                   token.type == TokenType::REDIR_APPEND;
        }
        bool hasShellOperatorTokens(const token_list_t &tokens)
        {
            for (const auto &token : tokens)
            {
                if (Lang::isShellOperatorToken(token))
                {
                    return true;
                }
            }
            return false;
        }
    } // namespace Lang
} // namespace EmbeddedTerminal
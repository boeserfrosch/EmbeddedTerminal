#pragma once
#include "Lexer.h"

namespace EmbeddedTerminal
{
    namespace Lang
    {
        // Token helper utilities used across subsystems. Placed here to avoid
        // cyclic dependencies between scripting and exec.
        bool isReservedWordToken(const TokenType type);
        bool isComparisonOperatorToken(const TokenType type);
        bool isLogicalOperatorToken(const TokenType type);
        bool isBinaryOperatorToken(const TokenType type);
        bool isUnaryOperatorToken(const TokenType type);

        bool isSeparator(const TokenType type);
        bool isStopToken(const TokenType type, const std::vector<TokenType> &stopAt);
        bool isShellOperatorToken(const token_t &token);
        bool hasShellOperatorTokens(const token_list_t &tokens);
    }
}

#ifndef LEXER_H
#define LEXER_H

#include "ETTypes.h"

namespace EmbeddedTerminal
{
    enum class TokenType
    {
        WORD,
        PIPE,
        REDIR_OUT,
        REDIR_IN,
        REDIR_APPEND,
        SEMI,
        AND_AND,
        OR_OR,
        NEWLINE,
        END_OF_FILE
    };

    struct token_t
    {
        TokenType type;
        ETString text;
        bool singleQuoted = false;
        bool doubleQuoted = false;
    };

    enum class LexerError
    {
        NONE = 0,
        TOKEN_ARRAY_EXHAUSTED,
        UNTERMINATED_SINGLE_QUOTE,
        UNTERMINATED_DOUBLE_QUOTE
    };

    LexerError lex(const ETString &input, token_t tokens[], size_t tokenCapacity, size_t &tokenCount);
} // namespace EmbeddedTerminal

#endif // LEXER_H

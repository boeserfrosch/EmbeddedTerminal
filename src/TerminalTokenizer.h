#ifndef TERMINAL_TOKENIZER_H
#define TERMINAL_TOKENIZER_H

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

    ETString toString(TokenType type);

    struct token_t
    {
        TokenType type;
        ETString text;
        bool singleQuoted = false;
        bool doubleQuoted = false;
    };

    struct token_spec_t
    {
        TokenType type;
        const char *text;
    };

    const token_spec_t *knownTokens(size_t &count);

    enum class LexerError
    {
        NONE = 0,
        TOKEN_ARRAY_EXHAUSTED,
        UNTERMINATED_SINGLE_QUOTE,
        UNTERMINATED_DOUBLE_QUOTE
    };

    class TerminalTokenizer
    {
    public:
        bool tokenizeLine(const ETString &line, ETVector<token_t> &tokens, LexerError &lexerError) const;

    private:
        static constexpr size_t TOKEN_CAPACITY = 64;
    };
}

#endif // TERMINAL_TOKENIZER_H

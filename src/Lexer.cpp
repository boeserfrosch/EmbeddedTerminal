#include "Lexer.h"

namespace
{
    bool appendToken(EmbeddedTerminal::token_t tokens[], size_t tokenCapacity, size_t &tokenCount,
                     EmbeddedTerminal::TokenType type, const ETString &text = "", bool singleQuoted = false,
                     bool doubleQuoted = false)
    {
        if (tokenCount >= tokenCapacity)
        {
            return false;
        }

        tokens[tokenCount++] = {type, text, singleQuoted, doubleQuoted};
        return true;
    }

    bool isWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\r';
    }

    bool isOperatorChar(char c)
    {
        return c == '|' || c == '>' || c == '<' || c == ';' || c == '&' || c == '\n';
    }
} // namespace

EmbeddedTerminal::LexerError EmbeddedTerminal::lex(const ETString &input, token_t tokens[], size_t tokenCapacity, size_t &tokenCount)
{
    tokenCount = 0;

    size_t index = 0;
    while (index < input.length())
    {
        const char current = input[index];

        if (isWhitespace(current))
        {
            index++;
            continue;
        }

        if (current == '\n')
        {
            if (!appendToken(tokens, tokenCapacity, tokenCount, TokenType::NEWLINE))
            {
                return LexerError::TOKEN_ARRAY_EXHAUSTED;
            }
            index++;
            continue;
        }

        if (current == '|')
        {
            if (index + 1 < input.length() && input[index + 1] == '|')
            {
                if (!appendToken(tokens, tokenCapacity, tokenCount, TokenType::OR_OR))
                {
                    return LexerError::TOKEN_ARRAY_EXHAUSTED;
                }
                index += 2;
            }
            else
            {
                if (!appendToken(tokens, tokenCapacity, tokenCount, TokenType::PIPE))
                {
                    return LexerError::TOKEN_ARRAY_EXHAUSTED;
                }
                index++;
            }
            continue;
        }

        if (current == '&')
        {
            if (index + 1 < input.length() && input[index + 1] == '&')
            {
                if (!appendToken(tokens, tokenCapacity, tokenCount, TokenType::AND_AND))
                {
                    return LexerError::TOKEN_ARRAY_EXHAUSTED;
                }
                index += 2;
                continue;
            }
        }

        if (current == '>')
        {
            if (index + 1 < input.length() && input[index + 1] == '>')
            {
                if (!appendToken(tokens, tokenCapacity, tokenCount, TokenType::REDIR_APPEND))
                {
                    return LexerError::TOKEN_ARRAY_EXHAUSTED;
                }
                index += 2;
            }
            else
            {
                if (!appendToken(tokens, tokenCapacity, tokenCount, TokenType::REDIR_OUT))
                {
                    return LexerError::TOKEN_ARRAY_EXHAUSTED;
                }
                index++;
            }
            continue;
        }

        if (current == '<')
        {
            if (!appendToken(tokens, tokenCapacity, tokenCount, TokenType::REDIR_IN))
            {
                return LexerError::TOKEN_ARRAY_EXHAUSTED;
            }
            index++;
            continue;
        }

        if (current == ';')
        {
            if (!appendToken(tokens, tokenCapacity, tokenCount, TokenType::SEMI))
            {
                return LexerError::TOKEN_ARRAY_EXHAUSTED;
            }
            index++;
            continue;
        }

        ETString value;
        bool singleQuoted = false;
        bool doubleQuoted = false;

        while (index < input.length())
        {
            char wordChar = input[index];

            if (isWhitespace(wordChar) || isOperatorChar(wordChar))
            {
                break;
            }

            if (wordChar == '\'')
            {
                singleQuoted = true;
                index++;

                while (index < input.length() && input[index] != '\'')
                {
                    value += input[index++];
                }

                if (index >= input.length())
                {
                    return LexerError::UNTERMINATED_SINGLE_QUOTE;
                }

                index++;
                continue;
            }

            if (wordChar == '"')
            {
                doubleQuoted = true;
                index++;

                while (index < input.length() && input[index] != '"')
                {
                    value += input[index++];
                }

                if (index >= input.length())
                {
                    return LexerError::UNTERMINATED_DOUBLE_QUOTE;
                }

                index++;
                continue;
            }

            value += wordChar;
            index++;
        }

        if (!value.empty())
        {
            if (!appendToken(tokens, tokenCapacity, tokenCount, TokenType::WORD, value, singleQuoted, doubleQuoted))
            {
                return LexerError::TOKEN_ARRAY_EXHAUSTED;
            }
            continue;
        }

        index++;
    }

    if (!appendToken(tokens, tokenCapacity, tokenCount, TokenType::END_OF_FILE))
    {
        return LexerError::TOKEN_ARRAY_EXHAUSTED;
    }

    return LexerError::NONE;
}

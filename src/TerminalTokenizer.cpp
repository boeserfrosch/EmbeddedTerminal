#include "TerminalTokenizer.h"

namespace EmbeddedTerminal
{
    namespace
    {
        bool appendToken(token_t tokens[], size_t tokenCapacity, size_t &tokenCount,
                         TokenType type, const ETString &text = "", bool singleQuoted = false,
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
    }

    bool TerminalTokenizer::tokenizeLine(const ETString &line, ETVector<token_t> &tokens, LexerError &lexerError) const
    {
        token_t tokenArray[TOKEN_CAPACITY];
        size_t tokenCount = 0;

        size_t index = 0;
        while (index < line.length())
        {
            const char current = line[index];

            if (isWhitespace(current))
            {
                index++;
                continue;
            }

            if (current == '\n')
            {
                if (!appendToken(tokenArray, TOKEN_CAPACITY, tokenCount, TokenType::NEWLINE))
                {
                    lexerError = LexerError::TOKEN_ARRAY_EXHAUSTED;
                    return false;
                }
                index++;
                continue;
            }

            if (current == '|')
            {
                if (index + 1 < line.length() && line[index + 1] == '|')
                {
                    if (!appendToken(tokenArray, TOKEN_CAPACITY, tokenCount, TokenType::OR_OR))
                    {
                        lexerError = LexerError::TOKEN_ARRAY_EXHAUSTED;
                        return false;
                    }
                    index += 2;
                }
                else
                {
                    if (!appendToken(tokenArray, TOKEN_CAPACITY, tokenCount, TokenType::PIPE))
                    {
                        lexerError = LexerError::TOKEN_ARRAY_EXHAUSTED;
                        return false;
                    }
                    index++;
                }
                continue;
            }

            if (current == '&')
            {
                if (index + 1 < line.length() && line[index + 1] == '&')
                {
                    if (!appendToken(tokenArray, TOKEN_CAPACITY, tokenCount, TokenType::AND_AND))
                    {
                        lexerError = LexerError::TOKEN_ARRAY_EXHAUSTED;
                        return false;
                    }
                    index += 2;
                    continue;
                }
            }

            if (current == '>')
            {
                if (index + 1 < line.length() && line[index + 1] == '>')
                {
                    if (!appendToken(tokenArray, TOKEN_CAPACITY, tokenCount, TokenType::REDIR_APPEND))
                    {
                        lexerError = LexerError::TOKEN_ARRAY_EXHAUSTED;
                        return false;
                    }
                    index += 2;
                }
                else
                {
                    if (!appendToken(tokenArray, TOKEN_CAPACITY, tokenCount, TokenType::REDIR_OUT))
                    {
                        lexerError = LexerError::TOKEN_ARRAY_EXHAUSTED;
                        return false;
                    }
                    index++;
                }
                continue;
            }

            if (current == '<')
            {
                if (!appendToken(tokenArray, TOKEN_CAPACITY, tokenCount, TokenType::REDIR_IN))
                {
                    lexerError = LexerError::TOKEN_ARRAY_EXHAUSTED;
                    return false;
                }
                index++;
                continue;
            }

            if (current == ';')
            {
                if (!appendToken(tokenArray, TOKEN_CAPACITY, tokenCount, TokenType::SEMI))
                {
                    lexerError = LexerError::TOKEN_ARRAY_EXHAUSTED;
                    return false;
                }
                index++;
                continue;
            }

            ETString value;
            bool singleQuoted = false;
            bool doubleQuoted = false;

            while (index < line.length())
            {
                char wordChar = line[index];

                if (isWhitespace(wordChar) || isOperatorChar(wordChar))
                {
                    break;
                }

                if (wordChar == '\'')
                {
                    singleQuoted = true;
                    index++;

                    while (index < line.length() && line[index] != '\'')
                    {
                        value += line[index++];
                    }

                    if (index >= line.length())
                    {
                        lexerError = LexerError::UNTERMINATED_SINGLE_QUOTE;
                        return false;
                    }

                    index++;
                    continue;
                }

                if (wordChar == '"')
                {
                    doubleQuoted = true;
                    index++;

                    while (index < line.length() && line[index] != '"')
                    {
                        value += line[index++];
                    }

                    if (index >= line.length())
                    {
                        lexerError = LexerError::UNTERMINATED_DOUBLE_QUOTE;
                        return false;
                    }

                    index++;
                    continue;
                }

                value += wordChar;
                index++;
            }

            if (!value.empty())
            {
                if (!appendToken(tokenArray, TOKEN_CAPACITY, tokenCount, TokenType::WORD, value, singleQuoted, doubleQuoted))
                {
                    lexerError = LexerError::TOKEN_ARRAY_EXHAUSTED;
                    return false;
                }
                continue;
            }

            index++;
        }

        if (!appendToken(tokenArray, TOKEN_CAPACITY, tokenCount, TokenType::END_OF_FILE))
        {
            lexerError = LexerError::TOKEN_ARRAY_EXHAUSTED;
            return false;
        }

        lexerError = LexerError::NONE;

        tokens.clear();
        tokens.reserve(tokenCount);
        for (size_t i = 0; i < tokenCount; ++i)
        {
            tokens.push_back(tokenArray[i]);
        }

        return true;
    }
}

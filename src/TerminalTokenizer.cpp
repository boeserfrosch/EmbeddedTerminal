#include "TerminalTokenizer.h"

namespace EmbeddedTerminal
{
    ETString toString(TokenType type)
    {
        switch (type)
        {
        case TokenType::WORD:
            return "WORD";
        case TokenType::PIPE:
            return "PIPE";
        case TokenType::REDIR_OUT:
            return "REDIR_OUT";
        case TokenType::REDIR_IN:
            return "REDIR_IN";
        case TokenType::REDIR_APPEND:
            return "REDIR_APPEND";
        case TokenType::SEMI:
            return "SEMI";
        case TokenType::AND_AND:
            return "AND_AND";
        case TokenType::OR_OR:
            return "OR_OR";
        case TokenType::NEWLINE:
            return "NEWLINE";
        case TokenType::END_OF_FILE:
            return "END_OF_FILE";
        default:
            return "UNKNOWN";
        }
    }

    const token_spec_t *knownTokens(size_t &count)
    {
        static const token_spec_t tokens[] = {
            {TokenType::REDIR_APPEND, ">>"},
            {TokenType::OR_OR, "||"},
            {TokenType::AND_AND, "&&"},
            {TokenType::PIPE, "|"},
            {TokenType::REDIR_OUT, ">"},
            {TokenType::REDIR_IN, "<"},
            {TokenType::SEMI, ";"},
            {TokenType::NEWLINE, "\n"}};

        count = sizeof(tokens) / sizeof(tokens[0]);
        return tokens;
    }

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

            token_t &token = tokens[tokenCount++];
            token.type = type;
            token.text = text;
            token.singleQuoted = singleQuoted;
            token.doubleQuoted = doubleQuoted;
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

            // We use knownTokens for single-character and multi-character operators to ensure consistent token text and simplify logic
            size_t knownTokenCount = 0;
            const token_spec_t *knownTokenTable = knownTokens(knownTokenCount);
            for (size_t knownTokenIndex = 0; knownTokenIndex < knownTokenCount; ++knownTokenIndex)
            {
                const token_spec_t &knownToken = knownTokenTable[knownTokenIndex];
                const ETString tokenText = knownToken.text;
                if (line.substr(index, tokenText.length()) == tokenText)
                {
                    appendToken(tokenArray, TOKEN_CAPACITY, tokenCount, knownToken.type, tokenText);
                    index += tokenText.length();
                    break;
                }
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

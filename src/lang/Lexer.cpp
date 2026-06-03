#include "Lexer.h"
#include <string>
#include <cstring>

namespace EmbeddedTerminal
{
    namespace Lang
    {

        ETVector<token_t> Lexer::tokenize(const ETString &input)
        {
            bool inToken = false;
            // bool inSingleQuote = false;
            // bool inDoubleQuote = false;

            size_t wordStart = 0;
            token_t token;
            ETVector<token_t> tokens;
            for (size_t i = 0; i < input.length(); ++i)
            {
                char current = input[i];

                switch (current)
                {
                case ' ':
                case '\t':
                case '\r':
                    if (!inToken)
                    {
                        continue; // Skip spaces between tokens
                    }
                    inToken = false;
                    handleFinishedWordToken(input, wordStart, i, tokens);
                    continue;
                case '$':
                    if (inToken)
                    {
                        continue; // Don't start a new token if we're already in one
                    }
                    wordStart = i;
                    i = findEndOfVariable(input, i);

                    token.text = input.substr(wordStart + 1, i - wordStart);
                    token.type = TokenType::VARIABLE;
                    token.startIndex = wordStart;
                    token.endIndex = i + 1;
                    tokens.push_back(token);
                    continue;
                case '/':
                    if (i + 1 < input.length() && input[i + 1] == '/')
                    {
                        wordStart = i;
                        i = findEndOfComment(input, i, "\n");

                        // tokens.push_back({input.substr(wordStart, i - wordStart), TokenType::ONELINE_COMMENT});
                        // tokens.push_back({"\n", TokenType::NEWLINE});
                    }
                    else if (i + 1 < input.length() && input[i + 1] == '*')
                    {
                        // Block comment
                        wordStart = i;
                        i = findEndOfComment(input, i, "*/");
                        // tokens.push_back({input.substr(wordStart, i - wordStart), TokenType::BLOCK_COMMENT});
                    }
                    else
                    {
                        // "/" is not followed by "/" or "*", so treat it as a regular word token
                        if (!inToken)
                        {
                            inToken = true;
                            wordStart = i;
                        }
                    }
                    continue;
                case '\'':
                case '"':
                    if (inToken)
                    {
                        continue; // Don't start a new token if we're already in one
                    }
                    wordStart = i;
                    i = findEndOfQuote(input, i, current);
                    inToken = false;
                    token.text = input.substr(wordStart + 1, i - wordStart - 1); // Exclude the surrounding quotes from the token text
                    token.type = (current == '"') ? TokenType::DOUBLE_QUOTE_STRING_LITERAL : TokenType::SINGLE_QUOTE_STRING_LITERAL;
                    token.startIndex = wordStart;
                    token.endIndex = i + 1;
                    token.terminated = (i < input.length() && input[i] == current);
                    tokens.push_back(token);
                    continue;
                case '|':
                case '>':
                case '&':
                case '=':
                    if (inToken)
                    {
                        inToken = false;
                        handleFinishedWordToken(input, wordStart, i, tokens);
                    }
                    if (i + 1 < input.length() && input[i + 1] == current)
                    {
                        token.text = TokenText(input, i, 2);
                        token.type = doubleOperatorTokenType(current);
                        token.startIndex = i;
                        token.endIndex = i + 2;
                        tokens.push_back(token);
                        ++i;
                    }
                    else
                    {
                        addOperatorToken(input, i, operatorTokenType(current), tokens);
                    }
                    continue;
                case '<':
                case ';':
                case '(':
                case ')':
                case '[':
                case ']':
                case '{':
                case '}':
                case '\n':
                    if (inToken)
                    {
                        inToken = false;
                        handleFinishedWordToken(input, wordStart, i, tokens);
                    }
                    addOperatorToken(input, i, operatorTokenType(current), tokens);
                    continue;
                case '!':
                    if (inToken)
                    {
                        inToken = false;
                        handleFinishedWordToken(input, wordStart, i, tokens);
                    }
                    if (i + 1 < input.length() && input[i + 1] == '=')
                    {
                        token.text = TokenText(input, i, 2);
                        token.type = TokenType::NOT_EQUALS;
                        token.startIndex = i;
                        token.endIndex = i + 2;
                        tokens.push_back(token);
                        ++i;
                    }
                    else
                    {
                        addOperatorToken(input, i, operatorTokenType(current), tokens);
                    }
                    continue;
                default:
                    if (!inToken)
                    {
                        inToken = true;
                        wordStart = i;
                    }
                }
            }
            // Handle the last token if the input doesn't end with a space
            if (inToken)
            {
                handleFinishedWordToken(input, wordStart, input.length(), tokens);
            }

            token.terminated = true;
            token.type = TokenType::END_OF_FILE;
            token.text = "";
            token.startIndex = input.length();
            token.endIndex = input.length();
            tokens.push_back(token); // Add EOF token at the end

            // Materialize token text before returning so the token list owns its text even if the
            // caller passed a temporary input string.
            for (auto &tok : tokens)
            {
                tok.text = tok.text.materialize();
            }

            return tokens;
        }

        size_t Lexer::findEndOfQuote(const ETString &input, size_t startIndex, char quoteChar) const
        {
            bool escaped = false;
            for (size_t i = startIndex + 1; i < input.length(); ++i)
            {
                char current = input[i];
                if (escaped)
                {
                    escaped = false;
                    continue;
                }
                if (current == '\\')
                {
                    escaped = true;
                    continue;
                }
                if (current == quoteChar)
                {
                    return i;
                }
            }
            return input.length(); // If we reach the end without finding a closing quote, return the end of the string
        }

        size_t Lexer::findEndOfComment(const ETString &input, size_t startIndex, const ETString &delimiter) const
        {
            size_t delimLen = delimiter.length();
            for (size_t i = startIndex; i + delimLen <= input.length(); ++i)
            {
                bool matches = true;
                for (size_t j = 0; j < delimLen; ++j)
                {
                    if (input[i + j] != delimiter[j])
                    {
                        matches = false;
                        break;
                    }
                }
                if (matches)
                {
                    return i + delimLen - 1; // Return the index of the last character of the delimiter
                }
            }

            return input.length(); // If we reach the end without finding the delimiter, return the end of the string
        }

        size_t Lexer::findEndOfVariable(const ETString &input, size_t startIndex) const
        {
            for (size_t i = startIndex + 1; i < input.length(); ++i)
            {
                char current = input[i];
                if (!(isalnum(static_cast<unsigned char>(current)) || current == '_'))
                {
                    return i - 1; // Return the index of the last valid variable character
                }
            }
            return input.length() - 1; // If we reach the end, return the index of the last character
        }

        void Lexer::handleFinishedWordToken(const ETString &input, size_t tokenStart, size_t tokenEnd, token_list_t &tokens)
        {
            if (tokenStart < tokenEnd)
            {
                token_t token;
                token.text = input.substr(tokenStart, tokenEnd - tokenStart);
                token.type = reservedWordTokenType(token.text);
                token.startIndex = tokenStart;
                token.endIndex = tokenEnd;
                tokens.push_back(token);
            }
        }

        void Lexer::addOperatorToken(const ETString &input, size_t index, TokenType type, token_list_t &tokens)
        {
            token_t token;
            token.text = input.substr(index, 1);
            token.type = type;
            token.startIndex = index;
            token.endIndex = index + 1;
            tokens.push_back(token);
        }

        TokenType Lexer::operatorTokenType(char c) const
        {
            switch (c)
            {
            case '>':
                return TokenType::REDIR_OUT;
            case '<':
                return TokenType::REDIR_IN;
            case ';':
                return TokenType::SEMI;
            case '(':
                return TokenType::PAREN_OPEN;
            case ')':
                return TokenType::PAREN_CLOSE;
            case '[':
                return TokenType::SQUARE_OPEN;
            case ']':
                return TokenType::SQUARE_CLOSE;
            case '{':
                return TokenType::CURLY_OPEN;
            case '}':
                return TokenType::CURLY_CLOSE;
            case '|':
                return TokenType::PIPE;
            case '\n':
                return TokenType::NEWLINE;
            case '=':
                return TokenType::EQUALS;
            case '!':
                return TokenType::NOT;
            default:
                return TokenType::WORD; // This should not happen for operator characters
            }
        }

        TokenType Lexer::doubleOperatorTokenType(char first) const
        {
            switch (first)
            {
            case '=':
                return TokenType::EQUALS_EQUALS;
            case '|':
                return TokenType::OR_OR;
            case '&':
                return TokenType::AND_AND;
            case '>':
                return TokenType::REDIR_APPEND;
            default:
                return TokenType::WORD; // This should not happen for operator characters
            }
        }

        TokenType Lexer::reservedWordTokenType(const TokenText &text) const
        {
            if (text.length() == 5 && text[0] == 'w' && text[1] == 'h' && text[2] == 'i' && text[3] == 'l' && text[4] == 'e')
                return TokenType::WHILE;
            if (text.length() == 3 && text[0] == 'f' && text[1] == 'o' && text[2] == 'r')
                return TokenType::FOR;
            if (text.length() == 2 && text[0] == 'i' && text[1] == 'f')
                return TokenType::IF;
            if (text.length() == 4 && text[0] == 't' && text[1] == 'h' && text[2] == 'e' && text[3] == 'n')
                return TokenType::THEN;
            if (text.length() == 4 && text[0] == 'e' && text[1] == 'l' && text[2] == 's' && text[3] == 'e')
                return TokenType::ELSE;
            if (text.length() == 4 && text[0] == 'e' && text[1] == 'l' && text[2] == 'i' && text[3] == 'f')
                return TokenType::ELIF;
            if (text.length() == 2 && text[0] == 'f' && text[1] == 'i')
                return TokenType::FI;
            if (text.length() == 2 && text[0] == 'd' && text[1] == 'o')
                return TokenType::DO;
            if (text.length() == 4 && text[0] == 'd' && text[1] == 'o' && text[2] == 'n' && text[3] == 'e')
                return TokenType::DONE;
            if (text.length() == 4 && text[0] == 't' && text[1] == 'r' && text[2] == 'u' && text[3] == 'e')
                return TokenType::TRUE;
            if (text.length() == 5 && text[0] == 'f' && text[1] == 'a' && text[2] == 'l' && text[3] == 's' && text[4] == 'e')
                return TokenType::FALSE;
            if (text.length() == 8 && text[0] == 'f' && text[1] == 'u' && text[2] == 'n' && text[3] == 'c' && text[4] == 't' && text[5] == 'i' && text[6] == 'o' && text[7] == 'n')
                return TokenType::FUNCTION;

            return TokenType::WORD; // Not a reserved word, treat as normal word token
        }

        ETVector<ETString> toTextVector(const token_list_t &tokens)
        {
            ETVector<ETString> result;
            for (size_t i = 0; i < tokens.size(); ++i)
            {
                if (tokens[i].type != TokenType::NEWLINE && tokens[i].type != TokenType::END_OF_FILE)
                {
                    result.push_back(tokens[i].text);
                }
            }
            return result;
        }

        size_t getNextTokenIndex(const token_list_t &tokens, TokenType targetType, size_t startIndex)
        {
            for (size_t i = startIndex; i < tokens.size(); ++i)
            {
                if (tokens[i].type == targetType)
                {
                    return i;
                }
            }
            return ETString::npos;
        }

        size_t getNextTokenIndexNotOfType(const token_list_t &tokens, const TokenType &excludedType, size_t startIndex)
        {
            for (size_t i = startIndex; i < tokens.size(); ++i)
            {
                if (tokens[i].type != excludedType)
                {
                    return i;
                }
            }
            return ETString::npos;
        }
    }
}
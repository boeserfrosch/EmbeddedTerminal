#pragma once
#include "ETTypes.h"
#include <cstring>
#include <string>

namespace EmbeddedTerminal
{
    namespace Lang
    {
        enum class TokenType
        {
            // GENERIC TOKENS
            WORD,
            DOUBLE_QUOTE_STRING_LITERAL,
            SINGLE_QUOTE_STRING_LITERAL,
            // Variables are emitted as separate tokens starting with the '$' character, so that they can be easily distinguished from regular words and processed accordingly during parsing and execution
            VARIABLE,
            // TERMINAL OPERATORS
            PIPE,
            REDIR_OUT,
            REDIR_IN,
            REDIR_APPEND,
            // LOGICAL OPERATORS
            AND_AND,
            OR_OR,
            EQUALS_EQUALS,
            NOT,
            NOT_EQUALS,
            // PARENTHESIS AND SEPARATORS
            PAREN_OPEN,
            PAREN_CLOSE,
            SQUARE_OPEN,
            SQUARE_CLOSE,
            CURLY_OPEN,
            CURLY_CLOSE,
            SEMI,
            NEWLINE,
            END_OF_FILE,
            EQUALS,
            // RESERVED WORDS
            WHILE,
            FOR,
            IF,
            THEN,
            ELSE,
            ELIF,
            FI,
            DO,
            DONE,
            FUNCTION,
            // BOOLEAN LITERALS
            TRUE,
            FALSE,

            // Comments (not emitted as tokens, but used internally during tokenization to skip comments)
            ONELINE_COMMENT,
            BLOCK_COMMENT,
            // Special token type used to indicate an invalid or uninitialized token, useful for error handling and default values
            NIL
        };

        class TokenText
        {
        public:
            TokenText() = default;
            TokenText(const TokenText &) = default;
            TokenText(TokenText &&) = default;
            TokenText(const ETString &text) : ownedText_(text) {}
            TokenText(const char *text) : ownedText_(text) {}
            TokenText(char text) : ownedText_(text) {}
            TokenText(const ETString &source, size_t startIndex, size_t length)
                : source_(&source), startIndex_(startIndex), length_(length), isSlice_(true)
            {
            }

            TokenText &operator=(const ETString &text)
            {
                ownedText_ = text;
                clearSlice_();
                return *this;
            }

            TokenText &operator=(const char *text)
            {
                ownedText_ = text;
                clearSlice_();
                return *this;
            }

            TokenText &operator=(char text)
            {
                ownedText_ = text;
                clearSlice_();
                return *this;
            }

            TokenText &operator=(const TokenText &) = default;

            size_t length() const
            {
                return isSlice_ ? length_ : ownedText_.length();
            }

            bool empty() const
            {
                return length() == 0;
            }

            char operator[](size_t index) const
            {
                return isSlice_ ? (*source_)[startIndex_ + index] : ownedText_[index];
            }

            bool operator==(const TokenText &other) const
            {
                if (length() != other.length())
                {
                    return false;
                }
                return *this == other.c_str();
            }

            bool operator==(const ETString &other) const
            {
                return *this == other.c_str();
            }
            bool operator==(const char *other) const
            {
                auto len = strlen(other);
                if (length() != len)
                {
                    return false;
                }
                for (size_t i = 0; i < len; ++i)
                {
                    if ((*this)[i] != other[i])
                    {
                        return false;
                    }
                }
                return true;
            }

            ETString materialize() const
            {
                if (!isSlice_)
                {
                    return ownedText_;
                }
                return source_->substr(startIndex_, length_);
            }

            const char *c_str() const
            {
                ensureOwned_();
                return ownedText_.c_str();
            }

            operator ETString() const
            {
                return materialize();
            }

            size_t find(const ETString &needle, size_t pos = 0) const
            {
                return materialize().find(needle, pos);
            }

            size_t find(char needle, size_t pos = 0) const
            {
                if (!isSlice_)
                {
                    return ownedText_.find(needle, pos);
                }

                for (size_t index = pos; index < length_; ++index)
                {
                    if ((*source_)[startIndex_ + index] == needle)
                    {
                        return index;
                    }
                }
                return ETString::npos;
            }

            bool contains(const ETString &needle) const
            {
                return find(needle) != ETString::npos;
            }

            bool contains(char needle) const
            {
                return find(needle) != ETString::npos;
            }

            bool startsWith(char prefix) const
            {
                return !empty() && (*this)[0] == prefix;
            }

            bool endsWith(char suffix) const
            {
                return !empty() && (*this)[length() - 1] == suffix;
            }

            TokenText toLowerCase() const
            {
                return TokenText(materialize().toLowerCase());
            }

            bool operator!=(const TokenText &other) const
            {
                return !(*this == other);
            }

            bool operator!=(const ETString &other) const
            {
                return !(*this == other);
            }

            bool operator!=(const char *other) const
            {
                return !(*this == other);
            }

            bool operator<(const TokenText &other) const
            {
                return materialize() < other.materialize();
            }

            bool operator>(const TokenText &other) const
            {
                return materialize() > other.materialize();
            }

            bool operator<=(const TokenText &other) const
            {
                return materialize() == ETString(other.materialize());
            }

            bool operator>=(const TokenText &other) const
            {
                return materialize() >= other.materialize();
            }

            TokenText &operator+=(const ETString &other)
            {
                ensureOwned_();
                ownedText_ += other;
                return *this;
            }

            TokenText &operator+=(char other)
            {
                ensureOwned_();
                ownedText_ += other;
                return *this;
            }

        private:
            void clearSlice_()
            {
                source_ = nullptr;
                startIndex_ = 0;
                length_ = 0;
                isSlice_ = false;
            }

            void ensureOwned_() const
            {
                if (!isSlice_)
                {
                    return;
                }

                ownedText_ = source_->substr(startIndex_, length_);
                source_ = nullptr;
                startIndex_ = 0;
                length_ = 0;
                isSlice_ = false;
            }

            mutable ETString ownedText_;
            mutable const ETString *source_ = nullptr;
            mutable size_t startIndex_ = 0;
            mutable size_t length_ = 0;
            mutable bool isSlice_ = false;
        };

        struct token_t
        {
            TokenText text;
            TokenType type;
            bool terminated = true; // Used internally during tokenization to mark tokens that have been fully parsed
            size_t startIndex = static_cast<size_t>(-1);
            size_t endIndex = static_cast<size_t>(-1); // Exclusive end index in the source string

            token_t() : text(""), type(TokenType::NIL) {}
            token_t(const ETString &t, TokenType ty) : text(t), type(ty) {}
            token_t(const char *t, TokenType ty) : text(t), type(ty) {}
            token_t(const ETString &t, TokenType ty, bool term) : text(t), type(ty), terminated(term) {}

            static token_t createVariable(const ETString &variableName)
            {
                return token_t(variableName, TokenType::VARIABLE);
            }
        };

        typedef ETVector<token_t> token_list_t;

        ETVector<ETString> toTextVector(const token_list_t &tokens);

        size_t getNextTokenIndex(const token_list_t &tokens, TokenType targetType, size_t startIndex);
        size_t getNextTokenIndexNotOfType(const token_list_t &tokens, const TokenType &excludedType, size_t startIndex);

        enum class LexerError
        {
            NONE
        };

        class Lexer
        {
        public:
            token_list_t tokenize(const ETString &input);

        private:
            void handleFinishedWordToken(const ETString &input, size_t tokenStart, size_t tokenEnd, token_list_t &tokens);
            size_t findEndOfQuote(const ETString &input, size_t startIndex, char quoteChar) const;
            size_t findEndOfComment(const ETString &input, size_t startIndex, const ETString &delimiter) const;
            size_t findEndOfVariable(const ETString &input, size_t startIndex) const;
            void addOperatorToken(const ETString &input, size_t index, TokenType type, token_list_t &tokens);
            inline TokenType operatorTokenType(char c) const;
            inline TokenType doubleOperatorTokenType(char first) const;
            inline TokenType reservedWordTokenType(const TokenText &text) const;
        };
    } // namespace Lang
} // namespace EmbeddedTerminal

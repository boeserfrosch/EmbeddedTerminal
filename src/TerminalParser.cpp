#include "TerminalParser.h"

namespace EmbeddedTerminal
{
    bool TerminalParser::tokenToCommandText_(const token_t &token, ETString &out) const
    {
        switch (token.type)
        {
        case TokenType::WORD:
            out = token.text;
            return true;
        case TokenType::PIPE:
            out = "|";
            return true;
        case TokenType::REDIR_OUT:
            out = ">";
            return true;
        case TokenType::REDIR_IN:
            out = "<";
            return true;
        case TokenType::REDIR_APPEND:
            out = ">>";
            return true;
        case TokenType::AND_AND:
            out = "&&";
            return true;
        case TokenType::OR_OR:
            out = "||";
            return true;
        default:
            out = "";
            return false;
        }
    }

    void TerminalParser::appendWithSpace_(ETString &target, const ETString &text) const
    {
        if (text.empty())
        {
            return;
        }

        if (!target.empty())
        {
            target += " ";
        }
        target += text;
    }

    bool TerminalParser::parseCommandTokens_(const ETVector<token_t> &tokens, size_t start, size_t end, ParsedCommand &out) const
    {
        if (start > end || end >= tokens.size())
        {
            return false;
        }

        out.keywords.clear();
        out.arguments.clear();
        out.redirectOutPath = "";
        out.appendRedirect = false;
        out.redirectInPath = "";

        ETString currentKeyword;
        ETString currentArguments;

        for (size_t index = start; index <= end; ++index)
        {
            const token_t &token = tokens[index];

            if (token.type == TokenType::PIPE)
            {
                if (currentKeyword.empty())
                {
                    return false;
                }

                out.keywords.push_back(currentKeyword);
                out.arguments.push_back(currentArguments);
                currentKeyword = "";
                currentArguments = "";
                continue;
            }

            if (token.type == TokenType::REDIR_OUT || token.type == TokenType::REDIR_APPEND)
            {
                if (currentKeyword.empty())
                {
                    return false;
                }

                out.appendRedirect = (token.type == TokenType::REDIR_APPEND);
                ++index;
                if (index > end || tokens[index].type != TokenType::WORD)
                {
                    return false;
                }

                out.redirectOutPath = tokens[index].text;
                continue;
            }

            if (token.type == TokenType::REDIR_IN)
            {
                if (currentKeyword.empty())
                {
                    return false;
                }

                ++index;
                if (index > end || tokens[index].type != TokenType::WORD)
                {
                    return false;
                }

                out.redirectInPath = tokens[index].text;
                continue;
            }

            if (currentKeyword.empty())
            {
                if (token.type != TokenType::WORD)
                {
                    return false;
                }

                currentKeyword = token.text;
                continue;
            }

            ETString tokenText;
            if (!tokenToCommandText_(token, tokenText))
            {
                return false;
            }
            appendWithSpace_(currentArguments, tokenText);
        }

        if (currentKeyword.empty())
        {
            return false;
        }

        out.keywords.push_back(currentKeyword);
        out.arguments.push_back(currentArguments);
        return !out.keywords.empty();
    }

    bool TerminalParser::parseChainTokens_(const ETVector<token_t> &tokens, size_t start, size_t end, ParsedChain &out) const
    {
        if (start > end || end >= tokens.size())
        {
            return false;
        }

        out.segments.clear();
        ChainCondition nextCondition = ChainCondition::Always;
        size_t segmentStart = start;

        for (size_t index = start; index <= end; ++index)
        {
            TokenType type = tokens[index].type;
            if (type != TokenType::SEMI && type != TokenType::AND_AND && type != TokenType::OR_OR)
            {
                continue;
            }

            if (index == segmentStart)
            {
                return false;
            }

            ParsedChainSegment segment;
            segment.condition = nextCondition;
            if (!parseCommandTokens_(tokens, segmentStart, index - 1, segment.command))
            {
                return false;
            }
            out.segments.push_back(segment);

            if (type == TokenType::AND_AND)
            {
                nextCondition = ChainCondition::OnSuccess;
            }
            else if (type == TokenType::OR_OR)
            {
                nextCondition = ChainCondition::OnFailure;
            }
            else
            {
                nextCondition = ChainCondition::Always;
            }

            segmentStart = index + 1;
        }

        if (segmentStart > end)
        {
            return false;
        }

        ParsedChainSegment lastSegment;
        lastSegment.condition = nextCondition;
        if (!parseCommandTokens_(tokens, segmentStart, end, lastSegment.command))
        {
            return false;
        }
        out.segments.push_back(lastSegment);

        return !out.segments.empty();
    }

    bool TerminalParser::parseForLoopTokens_(const ETVector<token_t> &tokens, size_t start, size_t end, ParsedForLoop &out) const
    {
        if (start > end || end >= tokens.size())
        {
            return false;
        }

        size_t index = start;
        if (tokens[index].type != TokenType::WORD || tokens[index].text != "for")
        {
            return false;
        }
        ++index;

        if (index > end || tokens[index].type != TokenType::WORD)
        {
            return false;
        }
        out.variable = tokens[index].text;
        ++index;

        if (index > end || tokens[index].type != TokenType::WORD || tokens[index].text != "in")
        {
            return false;
        }
        ++index;

        out.values.clear();
        while (index <= end)
        {
            if (tokens[index].type == TokenType::SEMI)
            {
                ++index;
                continue;
            }

            if (tokens[index].type == TokenType::WORD && tokens[index].text == "do")
            {
                ++index;
                break;
            }

            if (tokens[index].type != TokenType::WORD)
            {
                return false;
            }

            out.values.push_back(tokens[index].text);
            ++index;
        }

        if (out.values.empty() || index > end)
        {
            return false;
        }

        size_t doneIndex = end + 1;
        for (size_t bodyIndex = index; bodyIndex <= end; ++bodyIndex)
        {
            if (tokens[bodyIndex].type == TokenType::WORD && tokens[bodyIndex].text == "done")
            {
                doneIndex = bodyIndex;
                break;
            }
        }

        if (doneIndex == end + 1)
        {
            return false;
        }

        size_t bodyEnd = doneIndex - 1;
        while (bodyEnd >= index && tokens[bodyEnd].type == TokenType::SEMI)
        {
            if (bodyEnd == 0)
            {
                break;
            }
            --bodyEnd;
        }

        if (bodyEnd < index)
        {
            return false;
        }

        for (size_t tail = doneIndex + 1; tail <= end; ++tail)
        {
            if (tokens[tail].type != TokenType::SEMI)
            {
                return false;
            }
        }

        return parseChainTokens_(tokens, index, bodyEnd, out.body);
    }

    bool TerminalParser::parseTokens(const ETVector<token_t> &tokens, ParsedAst &ast) const
    {
        if (tokens.empty())
        {
            return false;
        }

        size_t start = 0;
        while (start < tokens.size() && tokens[start].type == TokenType::NEWLINE)
        {
            ++start;
        }

        if (start >= tokens.size() || tokens[start].type == TokenType::END_OF_FILE)
        {
            return false;
        }

        size_t end = start;
        while (end < tokens.size() && tokens[end].type != TokenType::END_OF_FILE && tokens[end].type != TokenType::NEWLINE)
        {
            ++end;
        }

        if (end == start)
        {
            return false;
        }
        --end;

        ast = ParsedAst{};
        if (tokens[start].type == TokenType::WORD && tokens[start].text == "for")
        {
            ast.isForLoop = true;
            return parseForLoopTokens_(tokens, start, end, ast.forLoop);
        }

        ast.isForLoop = false;
        return parseChainTokens_(tokens, start, end, ast.chain);
    }
}

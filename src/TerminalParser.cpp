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

    bool TerminalParser::parseWhileLoopTokens_(const ETVector<token_t> &tokens, size_t start, size_t end, ParsedWhileLoop &out) const
    {
        if (start > end || end >= tokens.size())
        {
            return false;
        }

        size_t index = start;
        if (tokens[index].type != TokenType::WORD || tokens[index].text != "while")
        {
            return false;
        }
        ++index;

        size_t doIndex = end + 1;
        for (size_t probe = index; probe <= end; ++probe)
        {
            if (tokens[probe].type == TokenType::WORD && tokens[probe].text == "do")
            {
                doIndex = probe;
                break;
            }
        }

        if (doIndex == end + 1)
        {
            return false;
        }

        if (doIndex <= index)
        {
            return false;
        }

        size_t conditionStart = index;
        size_t conditionEnd = doIndex - 1;
        while (conditionEnd >= conditionStart && tokens[conditionEnd].type == TokenType::SEMI)
        {
            if (conditionEnd == 0)
            {
                break;
            }
            --conditionEnd;
        }

        if (conditionEnd < conditionStart)
        {
            return false;
        }

        out.hasLiteralCondition = false;
        out.literalCondition = false;
        out.condition.segments.clear();

        if (conditionStart == conditionEnd && tokens[conditionStart].type == TokenType::WORD)
        {
            ETString literal = tokens[conditionStart].text;
            literal.toLowerCase();
            if (literal == "true" || literal == "1")
            {
                out.hasLiteralCondition = true;
                out.literalCondition = true;
            }
            else if (literal == "false" || literal == "0")
            {
                out.hasLiteralCondition = true;
                out.literalCondition = false;
            }
            else
            {
                if (!parseChainTokens_(tokens, conditionStart, conditionEnd, out.condition))
                {
                    return false;
                }
            }
        }
        else
        {
            if (!parseChainTokens_(tokens, conditionStart, conditionEnd, out.condition))
            {
                return false;
            }
        }

        index = doIndex + 1;

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

    bool TerminalParser::parseIfBlockTokens_(const ETVector<token_t> &tokens, size_t start, size_t end, ParsedIfBlock &out) const
    {
        if (start > end || end >= tokens.size())
        {
            return false;
        }

        size_t index = start;
        if (tokens[index].type != TokenType::WORD || tokens[index].text != "if")
        {
            return false;
        }
        ++index;

        auto trimTrailingSemis = [&](size_t startIndex, size_t endIndex, size_t &trimmedEnd) -> bool
        {
            if (endIndex < startIndex)
            {
                return false;
            }

            trimmedEnd = endIndex;
            while (trimmedEnd >= startIndex && tokens[trimmedEnd].type == TokenType::SEMI)
            {
                if (trimmedEnd == 0)
                {
                    break;
                }
                --trimmedEnd;
            }

            return trimmedEnd >= startIndex;
        };

        auto findThenIndex = [&](size_t fromIndex, size_t &thenIndex) -> bool
        {
            thenIndex = end + 1;
            for (size_t probe = fromIndex; probe <= end; ++probe)
            {
                if (tokens[probe].type == TokenType::WORD && tokens[probe].text == "then")
                {
                    thenIndex = probe;
                    return true;
                }
            }
            return false;
        };

        auto findBranchEnd = [&](size_t bodyStart, size_t &branchEnd, ETString &delimiter) -> bool
        {
            branchEnd = end + 1;
            delimiter = "";
            for (size_t probe = bodyStart; probe <= end; ++probe)
            {
                if (tokens[probe].type != TokenType::WORD)
                {
                    continue;
                }

                const ETString &word = tokens[probe].text;
                if (word == "elif" || word == "else" || word == "fi")
                {
                    branchEnd = probe;
                    delimiter = word;
                    return true;
                }
            }

            return false;
        };

        size_t thenIndex = end + 1;
        if (!findThenIndex(index, thenIndex) || thenIndex <= index)
        {
            return false;
        }

        size_t conditionEnd = thenIndex - 1;
        if (!trimTrailingSemis(index, conditionEnd, conditionEnd))
        {
            return false;
        }

        if (!parseChainTokens_(tokens, index, conditionEnd, out.condition))
        {
            return false;
        }

        out.elifConditions.clear();
        out.elifBodies.clear();
        out.hasElse = false;
        out.elseBody.segments.clear();

        size_t bodyStart = thenIndex + 1;
        if (bodyStart > end)
        {
            return false;
        }

        size_t branchEnd = end + 1;
        ETString delimiter;
        if (!findBranchEnd(bodyStart, branchEnd, delimiter) || branchEnd <= bodyStart)
        {
            return false;
        }

        size_t thenEnd = branchEnd - 1;
        if (!trimTrailingSemis(bodyStart, thenEnd, thenEnd))
        {
            return false;
        }

        if (!parseChainTokens_(tokens, bodyStart, thenEnd, out.thenBody))
        {
            return false;
        }

        size_t cursor = branchEnd;
        while (cursor <= end)
        {
            if (tokens[cursor].type != TokenType::WORD)
            {
                return false;
            }

            const ETString &word = tokens[cursor].text;
            if (word == "fi")
            {
                for (size_t tail = cursor + 1; tail <= end; ++tail)
                {
                    if (tokens[tail].type != TokenType::SEMI)
                    {
                        return false;
                    }
                }
                return true;
            }

            if (word == "else")
            {
                if (out.hasElse)
                {
                    return false;
                }

                size_t elseStart = cursor + 1;
                if (elseStart > end)
                {
                    return false;
                }

                size_t elseEndMarker = end + 1;
                for (size_t probe = elseStart; probe <= end; ++probe)
                {
                    if (tokens[probe].type == TokenType::WORD && tokens[probe].text == "fi")
                    {
                        elseEndMarker = probe;
                        break;
                    }
                }

                if (elseEndMarker == end + 1 || elseEndMarker <= elseStart)
                {
                    return false;
                }

                size_t elseEnd = elseEndMarker - 1;
                if (!trimTrailingSemis(elseStart, elseEnd, elseEnd))
                {
                    return false;
                }

                if (!parseChainTokens_(tokens, elseStart, elseEnd, out.elseBody))
                {
                    return false;
                }

                out.hasElse = true;
                cursor = elseEndMarker;
                continue;
            }

            if (word != "elif")
            {
                return false;
            }

            size_t elifConditionStart = cursor + 1;
            if (elifConditionStart > end)
            {
                return false;
            }

            size_t elifThenIndex = end + 1;
            if (!findThenIndex(elifConditionStart, elifThenIndex) || elifThenIndex <= elifConditionStart)
            {
                return false;
            }

            size_t elifConditionEnd = elifThenIndex - 1;
            if (!trimTrailingSemis(elifConditionStart, elifConditionEnd, elifConditionEnd))
            {
                return false;
            }

            ParsedChain elifCondition;
            if (!parseChainTokens_(tokens, elifConditionStart, elifConditionEnd, elifCondition))
            {
                return false;
            }

            size_t elifBodyStart = elifThenIndex + 1;
            if (elifBodyStart > end)
            {
                return false;
            }

            size_t elifBranchEnd = end + 1;
            ETString elifDelimiter;
            if (!findBranchEnd(elifBodyStart, elifBranchEnd, elifDelimiter) || elifBranchEnd <= elifBodyStart)
            {
                return false;
            }

            size_t elifBodyEnd = elifBranchEnd - 1;
            if (!trimTrailingSemis(elifBodyStart, elifBodyEnd, elifBodyEnd))
            {
                return false;
            }

            ParsedChain elifBody;
            if (!parseChainTokens_(tokens, elifBodyStart, elifBodyEnd, elifBody))
            {
                return false;
            }

            out.elifConditions.push_back(elifCondition);
            out.elifBodies.push_back(elifBody);
            cursor = elifBranchEnd;
        }

        return false;
    }

    bool TerminalParser::extractFunctionDefs(ETVector<token_t> &tokens, ETVector<ParsedFunctionDef> &outDefs) const
    {
        outDefs.clear();
        ETVector<token_t> clean;

        size_t i = 0;
        while (i < tokens.size())
        {
            const token_t &tok = tokens[i];

            if (tok.type == TokenType::END_OF_FILE || tok.type == TokenType::NEWLINE)
            {
                clean.push_back(tok);
                ++i;
                continue;
            }

            if (tok.type != TokenType::WORD || tok.text != "function")
            {
                clean.push_back(tok);
                ++i;
                continue;
            }

            size_t peek = i + 1;
            while (peek < tokens.size() && tokens[peek].type == TokenType::SEMI)
            {
                ++peek;
            }

            if (peek >= tokens.size() || tokens[peek].type != TokenType::WORD)
            {
                clean.push_back(tok);
                ++i;
                continue;
            }

            ETString funcName = tokens[peek].text;
            size_t afterName = peek + 1;
            while (afterName < tokens.size() && tokens[afterName].type == TokenType::SEMI)
            {
                ++afterName;
            }

            if (afterName >= tokens.size() || tokens[afterName].type != TokenType::WORD || tokens[afterName].text != "do")
            {
                clean.push_back(tok);
                ++i;
                continue;
            }

            size_t bodyStart = afterName + 1;
            int depth = 0;
            size_t doneIdx = tokens.size();
            for (size_t j = bodyStart; j < tokens.size(); ++j)
            {
                if (tokens[j].type == TokenType::WORD)
                {
                    if (tokens[j].text == "do")
                    {
                        ++depth;
                    }
                    else if (tokens[j].text == "done")
                    {
                        if (depth == 0)
                        {
                            doneIdx = j;
                            break;
                        }
                        --depth;
                    }
                }
            }

            if (doneIdx >= tokens.size())
            {
                clean.push_back(tok);
                ++i;
                continue;
            }

            size_t bodyEnd = doneIdx - 1;
            while (bodyEnd >= bodyStart && tokens[bodyEnd].type == TokenType::SEMI)
            {
                if (bodyEnd == 0)
                {
                    break;
                }
                --bodyEnd;
            }

            if (bodyEnd < bodyStart)
            {
                clean.push_back(tok);
                ++i;
                continue;
            }

            ParsedFunctionDef def;
            def.name = funcName;
            if (!parseChainTokens_(tokens, bodyStart, bodyEnd, def.body))
            {
                clean.push_back(tok);
                ++i;
                continue;
            }

            outDefs.push_back(def);

            i = doneIdx + 1;
            while (i < tokens.size() && tokens[i].type == TokenType::SEMI)
            {
                ++i;
            }
        }

        tokens = clean;
        return true;
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
            ast.isWhileLoop = false;
            ast.isIfBlock = false;
            return parseForLoopTokens_(tokens, start, end, ast.forLoop);
        }

        if (tokens[start].type == TokenType::WORD && tokens[start].text == "while")
        {
            ast.isForLoop = false;
            ast.isWhileLoop = true;
            ast.isIfBlock = false;
            return parseWhileLoopTokens_(tokens, start, end, ast.whileLoop);
        }

        if (tokens[start].type == TokenType::WORD && tokens[start].text == "if")
        {
            ast.isForLoop = false;
            ast.isWhileLoop = false;
            ast.isIfBlock = true;
            return parseIfBlockTokens_(tokens, start, end, ast.ifBlock);
        }

        ast.isForLoop = false;
        ast.isWhileLoop = false;
        ast.isIfBlock = false;
        return parseChainTokens_(tokens, start, end, ast.chain);
    }
}

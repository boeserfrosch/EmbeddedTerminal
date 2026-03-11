#include "ScriptRunner.h"
#include "TerminalParser.h"
#include "TerminalTokenizer.h"

namespace EmbeddedTerminal
{
    namespace
    {
        ETString replaceAll_(const ETString &input, const ETString &needle, const ETString &replacement)
        {
            if (needle.empty())
            {
                return input;
            }

            ETString result = input;
            size_t pos = 0;
            while ((pos = result.find(needle, pos)) != ETString::npos)
            {
                ETString left = result.substr(0, pos);
                ETString right = result.substr(pos + needle.length());
                result = left + replacement + right;
                pos += replacement.length();
            }

            return result;
        }

        ETString substituteLoopVariable_(const ETString &input, const ETString &variableName, const ETString &value,
                                         const ScriptRunner::VariableSubstituter &substituter)
        {
            if (substituter)
            {
                return substituter(input, variableName, value);
            }

            ETString result = input;
            result = replaceAll_(result, "${" + variableName + "}", value);
            result = replaceAll_(result, "$" + variableName, value);
            return result;
        }

        bool parseUnsignedMs_(const ETString &text, uint32_t &out)
        {
            ETString trimmed = text.trim();
            if (trimmed.empty())
            {
                return false;
            }

            uint64_t value = 0;
            for (size_t i = 0; i < trimmed.length(); ++i)
            {
                char current = trimmed[i];
                if (current < '0' || current > '9')
                {
                    return false;
                }

                value = (value * 10u) + static_cast<uint64_t>(current - '0');
                if (value > 0xFFFFFFFFu)
                {
                    return false;
                }
            }

            out = static_cast<uint32_t>(value);
            return true;
        }

        ETString lexerErrorMessage_(LexerError lexerError)
        {
            if (lexerError == LexerError::TOKEN_ARRAY_EXHAUSTED)
            {
                return "lexer error: token array exhausted";
            }
            if (lexerError == LexerError::UNTERMINATED_SINGLE_QUOTE)
            {
                return "lexer error: unterminated single quote";
            }
            if (lexerError == LexerError::UNTERMINATED_DOUBLE_QUOTE)
            {
                return "lexer error: unterminated double quote";
            }

            return "lexer error: invalid command syntax";
        }
    }

    ScriptRunner::ScriptRunner(CommandDispatcher commandDispatcher,
                               ExitCodeProvider exitCodeProvider,
                               CommandBusyProvider commandBusyProvider,
                               TimeProvider timeProvider,
                               VariableSubstituter substituter)
        : commandDispatcher_(commandDispatcher),
          exitCodeProvider_(exitCodeProvider),
          commandBusyProvider_(commandBusyProvider),
          timeProvider_(timeProvider),
          substituter_(substituter)
    {
    }

    bool ScriptRunner::enqueueScript(const ETString &scriptText, ETString &errorMessage)
    {
        if (hasActiveScript_)
        {
            errorMessage = "script error: script already running";
            return false;
        }

        ETString normalized = scriptText;
        for (size_t index = 0; index < normalized.length(); ++index)
        {
            if (normalized[index] == '\n' || normalized[index] == '\r')
            {
                normalized[index] = ';';
            }
        }

        ETString cleaned = normalized.cleanupString().trim();
        if (cleaned.empty())
        {
            errorMessage = "script error: empty script";
            return false;
        }

        TerminalTokenizer tokenizer;
        TerminalParser parser;

        ETVector<token_t> tokens;
        LexerError lexerError = LexerError::NONE;
        if (!tokenizer.tokenizeLine(cleaned, tokens, lexerError))
        {
            errorMessage = lexerErrorMessage_(lexerError);
            return false;
        }

        ETVector<ParsedFunctionDef> funcDefs;
        if (!parser.extractFunctionDefs(tokens, funcDefs))
        {
            errorMessage = "script error: invalid function definition";
            return false;
        }

        functionRegistry_.clear();
        for (size_t fIdx = 0; fIdx < funcDefs.size(); ++fIdx)
        {
            functionRegistry_[funcDefs[fIdx].name] = funcDefs[fIdx].body;
        }

        bool allTokensEof = true;
        for (size_t tIdx = 0; tIdx < tokens.size(); ++tIdx)
        {
            if (tokens[tIdx].type != TokenType::END_OF_FILE &&
                tokens[tIdx].type != TokenType::NEWLINE &&
                tokens[tIdx].type != TokenType::SEMI)
            {
                allTokensEof = false;
                break;
            }
        }

        if (allTokensEof)
        {
            if (funcDefs.empty())
            {
                errorMessage = "script error: empty script";
                return false;
            }
            return true;
        }

        ParsedAst ast;
        if (!parser.parseTokens(tokens, ast))
        {
            errorMessage = "script error: invalid script syntax";
            return false;
        }

        if (ast.isWhileLoop)
        {
            if (ast.whileLoop.body.segments.empty())
            {
                errorMessage = "script error: empty while body";
                return false;
            }

            hasActiveScript_ = true;
            isWhileLoop_ = true;
            whileHasLiteralCondition_ = ast.whileLoop.hasLiteralCondition;
            whileLiteralCondition_ = ast.whileLoop.literalCondition;
            whileConditionChain_ = ast.whileLoop.condition;
            whileConditionIndex_ = 0;
            whileInConditionPhase_ = true;
            activeScriptChain_ = ast.whileLoop.body;
            activeScriptIndex_ = 0;
            scriptDelayPending_ = false;
            scriptResumeAtMs_ = 0;
            return true;
        }

        if (ast.isIfBlock)
        {
            if (ast.ifBlock.condition.segments.empty() || ast.ifBlock.thenBody.segments.empty())
            {
                errorMessage = "script error: invalid if block";
                return false;
            }

            hasActiveScript_ = true;
            isWhileLoop_ = false;
            whileHasLiteralCondition_ = false;
            whileLiteralCondition_ = false;
            whileConditionChain_.segments.clear();
            whileConditionIndex_ = 0;
            whileInConditionPhase_ = false;

            isIfBlock_ = true;
            ifConditionChains_.clear();
            ifConditionIndices_.clear();
            ifBranchChains_.clear();

            ifConditionChains_.push_back(ast.ifBlock.condition);
            ifConditionIndices_.push_back(0);
            ifBranchChains_.push_back(ast.ifBlock.thenBody);

            for (size_t branchIndex = 0; branchIndex < ast.ifBlock.elifConditions.size(); ++branchIndex)
            {
                if (branchIndex >= ast.ifBlock.elifBodies.size())
                {
                    errorMessage = "script error: invalid if block";
                    return false;
                }

                ifConditionChains_.push_back(ast.ifBlock.elifConditions[branchIndex]);
                ifConditionIndices_.push_back(0);
                ifBranchChains_.push_back(ast.ifBlock.elifBodies[branchIndex]);
            }

            ifThenChain_.segments.clear();
            ifElseChain_ = ast.ifBlock.elseBody;
            ifHasElseChain_ = ast.ifBlock.hasElse;
            ifCurrentCondition_ = 0;
            ifPhase_ = IfPhase::Condition;

            activeScriptChain_.segments.clear();
            activeScriptIndex_ = 0;
            scriptDelayPending_ = false;
            scriptResumeAtMs_ = 0;
            return true;
        }

        ParsedChain expanded;
        if (!expandAstToChain_(ast, expanded))
        {
            errorMessage = "script error: failed to expand script";
            return false;
        }

        if (expanded.segments.empty())
        {
            errorMessage = "script error: failed to expand script";
            return false;
        }

        activeScriptChain_ = expanded;
        activeScriptIndex_ = 0;
        hasActiveScript_ = true;
        isWhileLoop_ = false;
        isIfBlock_ = false;
        whileHasLiteralCondition_ = false;
        whileLiteralCondition_ = false;
        whileConditionChain_.segments.clear();
        whileConditionIndex_ = 0;
        whileInConditionPhase_ = false;
        ifConditionChains_.clear();
        ifConditionIndices_.clear();
        ifBranchChains_.clear();
        ifCurrentCondition_ = 0;
        ifThenChain_.segments.clear();
        ifElseChain_.segments.clear();
        ifHasElseChain_ = false;
        ifPhase_ = IfPhase::None;
        scriptDelayPending_ = false;
        scriptResumeAtMs_ = 0;
        return true;
    }

    void ScriptRunner::tick()
    {
        if (!hasActiveScript_)
        {
            return;
        }

        if (commandBusyProvider_ && commandBusyProvider_())
        {
            return;
        }

        if (scriptDelayPending_)
        {
            if (!timeProvider_ || timeProvider_() < scriptResumeAtMs_)
            {
                return;
            }

            scriptDelayPending_ = false;
        }

        while (!callStack_.empty())
        {
            bool frameCompleted = false;
            if (executeChainStep_(callStack_.back().chain, callStack_.back().index, frameCompleted))
            {
                return;
            }

            if (frameCompleted)
            {
                callStack_.pop_back();
            }

            if (!callStack_.empty())
            {
                continue;
            }

            break;
        }

        if (!callStack_.empty())
        {
            return;
        }

        if (!isWhileLoop_)
        {
            if (isIfBlock_)
            {
                while (true)
                {
                    if (ifPhase_ == IfPhase::Condition)
                    {
                        if (ifCurrentCondition_ >= ifConditionChains_.size())
                        {
                            if (ifHasElseChain_)
                            {
                                activeScriptChain_ = ifElseChain_;
                                activeScriptIndex_ = 0;
                                ifPhase_ = IfPhase::Branch;
                                continue;
                            }

                            reset();
                            return;
                        }

                        bool conditionCompleted = false;
                        if (executeChainStep_(ifConditionChains_[ifCurrentCondition_], ifConditionIndices_[ifCurrentCondition_], conditionCompleted))
                        {
                            drainCallStack_();
                            return;
                        }

                        if (conditionCompleted)
                        {
                            bool conditionTrue = !exitCodeProvider_ || (exitCodeProvider_() == 0);
                            ifConditionIndices_[ifCurrentCondition_] = 0;

                            if (conditionTrue)
                            {
                                activeScriptChain_ = ifBranchChains_[ifCurrentCondition_];
                                activeScriptIndex_ = 0;
                                ifPhase_ = IfPhase::Branch;
                                continue;
                            }

                            ++ifCurrentCondition_;
                            continue;
                        }

                        return;
                    }

                    if (ifPhase_ == IfPhase::Branch)
                    {
                        bool branchCompleted = false;
                        if (executeChainStep_(activeScriptChain_, activeScriptIndex_, branchCompleted))
                        {
                            drainCallStack_();
                            return;
                        }

                        if (branchCompleted)
                        {
                            reset();
                        }
                        return;
                    }

                    reset();
                    return;
                }
            }

            bool completed = false;
            if (executeChainStep_(activeScriptChain_, activeScriptIndex_, completed))
            {
                drainCallStack_();
                return;
            }

            if (completed)
            {
                reset();
            }
            return;
        }

        while (true)
        {
            if (whileInConditionPhase_)
            {
                if (whileHasLiteralCondition_)
                {
                    if (!whileLiteralCondition_)
                    {
                        reset();
                        return;
                    }

                    whileInConditionPhase_ = false;
                    activeScriptIndex_ = 0;
                    continue;
                }

                bool conditionCompleted = false;
                if (executeChainStep_(whileConditionChain_, whileConditionIndex_, conditionCompleted))
                {
                    drainCallStack_();
                    return;
                }

                if (conditionCompleted)
                {
                    whileConditionIndex_ = 0;
                    bool conditionTrue = !exitCodeProvider_ || (exitCodeProvider_() == 0);
                    if (!conditionTrue)
                    {
                        reset();
                        return;
                    }

                    whileInConditionPhase_ = false;
                    activeScriptIndex_ = 0;
                    continue;
                }

                return;
            }

            bool bodyCompleted = false;
            if (executeChainStep_(activeScriptChain_, activeScriptIndex_, bodyCompleted))
            {
                drainCallStack_();
                return;
            }

            if (bodyCompleted)
            {
                activeScriptIndex_ = 0;
                whileInConditionPhase_ = true;
                continue;
            }

            return;
        }
    }

    void ScriptRunner::drainCallStack_()
    {
        while (!callStack_.empty())
        {
            bool frameCompleted = false;
            if (executeChainStep_(callStack_.back().chain, callStack_.back().index, frameCompleted))
            {
                return;
            }

            if (frameCompleted)
            {
                callStack_.pop_back();
            }

            if (!callStack_.empty())
            {
                continue;
            }

            break;
        }
    }

    bool ScriptRunner::executeChainStep_(const ParsedChain &chain, size_t &index, bool &completed)
    {
        completed = false;

        while (index < chain.segments.size())
        {
            const ParsedChainSegment &segment = chain.segments[index];
            bool shouldExecute = false;

            if (segment.condition == ChainCondition::Always)
            {
                shouldExecute = true;
            }
            else if (segment.condition == ChainCondition::OnSuccess)
            {
                shouldExecute = !exitCodeProvider_ || (exitCodeProvider_() == 0);
            }
            else
            {
                shouldExecute = !exitCodeProvider_ || (exitCodeProvider_() != 0);
            }

            ++index;

            if (!shouldExecute)
            {
                continue;
            }

            uint32_t delayMs = 0;
            if (parseDelayMs_(segment.command, delayMs))
            {
                if (delayMs > 0)
                {
                    scriptDelayPending_ = true;
                    scriptResumeAtMs_ = (timeProvider_ ? timeProvider_() : 0) + static_cast<uint64_t>(delayMs);
                    return true;
                }

                continue;
            }

            if (!segment.command.keywords.empty())
            {
                const ETString &kw = segment.command.keywords[0];
                auto funcIt = functionRegistry_.find(kw);
                if (funcIt != functionRegistry_.end() && callStack_.size() < kMaxCallDepth_)
                {
                    CallFrame_ frame;
                    frame.chain = funcIt->second;
                    frame.index = 0;
                    callStack_.push_back(frame);
                    return true;
                }
            }

            if (commandDispatcher_)
            {
                commandDispatcher_(segment.command);
            }

            return true;
        }

        completed = true;
        return false;
    }

    void ScriptRunner::reset()
    {
        hasActiveScript_ = false;
        isWhileLoop_ = false;
        isIfBlock_ = false;
        whileHasLiteralCondition_ = false;
        whileLiteralCondition_ = false;
        whileConditionChain_.segments.clear();
        whileConditionIndex_ = 0;
        whileInConditionPhase_ = false;
        ifConditionChains_.clear();
        ifConditionIndices_.clear();
        ifBranchChains_.clear();
        ifCurrentCondition_ = 0;
        ifThenChain_.segments.clear();
        ifElseChain_.segments.clear();
        ifHasElseChain_ = false;
        ifPhase_ = IfPhase::None;
        functionRegistry_.clear();
        callStack_.clear();
        activeScriptChain_.segments.clear();
        activeScriptIndex_ = 0;
        scriptDelayPending_ = false;
        scriptResumeAtMs_ = 0;
    }

    bool ScriptRunner::isActive() const
    {
        return hasActiveScript_;
    }

    bool ScriptRunner::expandAstToChain_(const ParsedAst &ast, ParsedChain &out) const
    {
        out.segments.clear();

        if (!ast.isForLoop)
        {
            out = ast.chain;
            return !out.segments.empty();
        }

        const ParsedForLoop &loop = ast.forLoop;
        if (loop.values.empty() || loop.body.segments.empty())
        {
            return false;
        }

        for (size_t valueIndex = 0; valueIndex < loop.values.size(); ++valueIndex)
        {
            const ETString &value = loop.values[valueIndex];
            ParsedChain expandedBody = loop.body;

            for (size_t segIndex = 0; segIndex < expandedBody.segments.size(); ++segIndex)
            {
                ParsedCommand &command = expandedBody.segments[segIndex].command;

                for (size_t i = 0; i < command.keywords.size(); ++i)
                {
                    command.keywords[i] = substituteLoopVariable_(command.keywords[i], loop.variable, value, substituter_);
                }
                for (size_t i = 0; i < command.arguments.size(); ++i)
                {
                    command.arguments[i] = substituteLoopVariable_(command.arguments[i], loop.variable, value, substituter_);
                }

                command.redirectOutPath = substituteLoopVariable_(command.redirectOutPath, loop.variable, value, substituter_);
                command.redirectInPath = substituteLoopVariable_(command.redirectInPath, loop.variable, value, substituter_);
            }

            out.segments.insert(out.segments.end(), expandedBody.segments.begin(), expandedBody.segments.end());
        }

        return !out.segments.empty();
    }

    bool ScriptRunner::parseDelayMs_(const ParsedCommand &command, uint32_t &outMs) const
    {
        if (command.keywords.size() != 1 || command.arguments.size() != 1)
        {
            return false;
        }

        if (!command.redirectOutPath.empty() || !command.redirectInPath.empty())
        {
            return false;
        }

        ETString keyword = command.keywords[0].trim();
        ETString argument = command.arguments[0].trim();

        if (keyword == "delay")
        {
            return parseUnsignedMs_(argument, outMs);
        }

        return false;
    }
}

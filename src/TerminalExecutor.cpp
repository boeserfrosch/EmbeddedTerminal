#include "TerminalExecutor.h"

namespace EmbeddedTerminal
{
    TerminalExecutor::TerminalExecutor(CommandExecutor commandExecutor, ExitCodeProvider exitCodeProvider, VariableSubstituter substituter, DelayExecutor delayExecutor)
        : commandExecutor_(commandExecutor), exitCodeProvider_(exitCodeProvider), substituter_(substituter), delayExecutor_(delayExecutor)
    {
    }

    void TerminalExecutor::execute(const ParsedAst &ast) const
    {
        if (ast.isForLoop)
        {
            executeForLoop_(ast.forLoop);
            return;
        }

        executeChain_(ast.chain);
    }

    void TerminalExecutor::executeChain_(const ParsedChain &chain) const
    {
        for (size_t index = 0; index < chain.segments.size(); ++index)
        {
            const ParsedChainSegment &segment = chain.segments[index];
            bool shouldExecute = false;

            if (segment.condition == ChainCondition::Always)
            {
                shouldExecute = true;
            }
            else if (segment.condition == ChainCondition::OnSuccess)
            {
                shouldExecute = (exitCodeProvider_() == 0);
            }
            else
            {
                shouldExecute = (exitCodeProvider_() != 0);
            }

            if (!shouldExecute)
            {
                continue;
            }

            uint32_t delayMilliseconds = 0;
            if (parseDelayMilliseconds_(segment.command, delayMilliseconds))
            {
                executeDelay_(delayMilliseconds);
                continue;
            }

            commandExecutor_(segment.command);
        }
    }

    bool TerminalExecutor::parseUnsignedMilliseconds_(const ETString &text, uint32_t &out) const
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

    bool TerminalExecutor::parseDelayMilliseconds_(const ParsedCommand &command, uint32_t &out) const
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
            return parseUnsignedMilliseconds_(argument, out);
        }

        return false;
    }

    void TerminalExecutor::executeDelay_(uint32_t milliseconds) const
    {
        if (!delayExecutor_)
        {
            return;
        }

        delayExecutor_(milliseconds);
    }

    void TerminalExecutor::executeForLoop_(const ParsedForLoop &loop) const
    {
        for (size_t valueIndex = 0; valueIndex < loop.values.size(); ++valueIndex)
        {
            ParsedChain expanded = loop.body;
            const ETString &value = loop.values[valueIndex];

            for (size_t segIndex = 0; segIndex < expanded.segments.size(); ++segIndex)
            {
                ParsedCommand &command = expanded.segments[segIndex].command;

                for (size_t i = 0; i < command.keywords.size(); ++i)
                {
                    command.keywords[i] = substituter_(command.keywords[i], loop.variable, value);
                }
                for (size_t i = 0; i < command.arguments.size(); ++i)
                {
                    command.arguments[i] = substituter_(command.arguments[i], loop.variable, value);
                }

                command.redirectOutPath = substituter_(command.redirectOutPath, loop.variable, value);
                command.redirectInPath = substituter_(command.redirectInPath, loop.variable, value);
            }

            executeChain_(expanded);
        }
    }
}

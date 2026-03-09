#include "TerminalExecutor.h"

namespace EmbeddedTerminal
{
    TerminalExecutor::TerminalExecutor(CommandExecutor commandExecutor, ExitCodeProvider exitCodeProvider, VariableSubstituter substituter)
        : commandExecutor_(commandExecutor), exitCodeProvider_(exitCodeProvider), substituter_(substituter)
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

            commandExecutor_(segment.command);
        }
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

#ifndef TERMINAL_EXECUTOR_H
#define TERMINAL_EXECUTOR_H

#include "TerminalAst.h"
#include <functional>

namespace EmbeddedTerminal
{
    class TerminalExecutor
    {
    public:
        using CommandExecutor = std::function<void(const ParsedCommand &)>;
        using ExitCodeProvider = std::function<int(void)>;
        using VariableSubstituter = std::function<ETString(const ETString &, const ETString &, const ETString &)>;

        TerminalExecutor(CommandExecutor commandExecutor, ExitCodeProvider exitCodeProvider, VariableSubstituter substituter);
        void execute(const ParsedAst &ast) const;

    private:
        void executeChain_(const ParsedChain &chain) const;
        void executeForLoop_(const ParsedForLoop &loop) const;

        CommandExecutor commandExecutor_;
        ExitCodeProvider exitCodeProvider_;
        VariableSubstituter substituter_;
    };
}

#endif // TERMINAL_EXECUTOR_H

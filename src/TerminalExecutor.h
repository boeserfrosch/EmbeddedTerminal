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
        using DelayExecutor = std::function<void(uint32_t)>;

        TerminalExecutor(CommandExecutor commandExecutor, ExitCodeProvider exitCodeProvider, VariableSubstituter substituter, DelayExecutor delayExecutor = DelayExecutor{});
        void execute(const ParsedAst &ast) const;

    private:
        void executeChain_(const ParsedChain &chain) const;
        void executeForLoop_(const ParsedForLoop &loop) const;
        bool parseUnsignedMilliseconds_(const ETString &text, uint32_t &out) const;
        bool parseDelayMilliseconds_(const ParsedCommand &command, uint32_t &out) const;
        void executeDelay_(uint32_t milliseconds) const;

        CommandExecutor commandExecutor_;
        ExitCodeProvider exitCodeProvider_;
        VariableSubstituter substituter_;
        DelayExecutor delayExecutor_;
    };
}

#endif // TERMINAL_EXECUTOR_H

#ifndef I_COMMAND_RUNTIME_H
#define I_COMMAND_RUNTIME_H

#include "../ETTypes.h"

namespace EmbeddedTerminal
{
    enum class CommandExecutionState
    {
        Completed,
        Running,
        WaitingForInput
    };

    struct CommandResult
    {
        int exitCode = 0;
        CommandExecutionState state = CommandExecutionState::Completed;

        static CommandResult completed(int code = 0)
        {
            CommandResult result;
            result.exitCode = code;
            result.state = CommandExecutionState::Completed;
            return result;
        }

        static CommandResult running(int code = 0)
        {
            CommandResult result;
            result.exitCode = code;
            result.state = CommandExecutionState::Running;
            return result;
        }

        static CommandResult waitingForInput(int code = 0)
        {
            CommandResult result;
            result.exitCode = code;
            result.state = CommandExecutionState::WaitingForInput;
            return result;
        }
    };

    class IInputChannel
    {
    public:
        virtual ~IInputChannel() = default;
        virtual bool available() = 0;
        virtual ETString readAll() = 0;
    };

    class IOutputChannel
    {
    public:
        virtual ~IOutputChannel() = default;
        virtual void print(const ETString &s) = 0;
    };

    struct CommandContext
    {
        ETMap<ETString, ETString> &variables;
        int lastExitCode = 0;
        bool interactive = false;

        CommandContext(ETMap<ETString, ETString> &vars, int exitCode = 0, bool isInteractive = false)
            : variables(vars), lastExitCode(exitCode), interactive(isInteractive)
        {
        }
    };

    struct CommandInvocation
    {
        const ETString &keyword;
        const ETString &arguments;
        CommandContext &context;
        IInputChannel &stdinChannel;
        IOutputChannel &stdoutChannel;
        IOutputChannel &stderrChannel;
    };
}

#endif // I_COMMAND_RUNTIME_H

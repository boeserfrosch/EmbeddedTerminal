#ifndef I_COMMAND_RUNTIME_H
#define I_COMMAND_RUNTIME_H

#include "../ETTypes.h"

namespace EmbeddedTerminal
{
    /**
     * @brief Interface for command execution state
     *
     * This interface defines the possible states of command execution
     */
    enum class CommandExecutionState
    {
        Completed,
        Running,
        WaitingForInput
    };

    /**
     * @brief Struct to represent the result of a command execution
     *
     * This struct contains the exit code and the execution state of a command after it has been executed.
     */
    struct CommandResult
    {
        int exitCode = 0;
        CommandExecutionState state = CommandExecutionState::Completed;

        /**
         * @brief Creates a completed command result.
         * @param code The exit code for the completed command.
         * @return A CommandResult with the specified exit code and Completed state.
         */
        static CommandResult completed(int code = 0)
        {
            CommandResult result;
            result.exitCode = code;
            result.state = CommandExecutionState::Completed;
            return result;
        }

        /**
         * @brief Creates a running command result.
         * @param code The exit code for the running command (default is 0).
         * @return A CommandResult with the specified exit code and Running state.
         */
        static CommandResult running(int code = 0)
        {
            CommandResult result;
            result.exitCode = code;
            result.state = CommandExecutionState::Running;
            return result;
        }

        /**
         * @brief Creates a waiting for input command result.
         * @param code The exit code for the waiting command (default is 0).
         * @return A CommandResult with the specified exit code and WaitingForInput state.
         */
        static CommandResult waitingForInput(int code = 0)
        {
            CommandResult result;
            result.exitCode = code;
            result.state = CommandExecutionState::WaitingForInput;
            return result;
        }
    };

    /**
     * @brief Interface for Input channel during command execution
     */
    class IInputChannel
    {
    public:
        virtual ~IInputChannel() = default;

        /**
         * @brief Checks if there is input available to read.
         * @return True if input is available, false otherwise.
         */
        virtual bool available() = 0;
        /**
         * @brief Reads all input from the channel.
         * @return The input read from the channel.
         */
        virtual ETString readAll() = 0;
    };

    /**
     * @brief Interface for Output channel during command execution
     *
     */
    class IOutputChannel
    {
    public:
        virtual ~IOutputChannel() = default;

        /**
         * @brief Prints a string to the output channel.
         * @param s The string to print.
         */
        virtual void print(const ETString &s) = 0;
    };

    /**
     * @brief Struct to represent the context of a command execution
     */
    struct CommandContext
    {

        /**
         * @brief A map of variables that can be used to store state across command executions
         */
        ETMap<ETString, ETString> &variables;

        /**
         * @brief The exit code of the last executed command
         */
        int lastExitCode = 0;

        /**
         *  @brief Whether the command is running in interactive mode
         * */
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

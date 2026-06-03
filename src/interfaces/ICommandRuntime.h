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
        int32_t exitCode = 0;
        CommandExecutionState state = CommandExecutionState::Completed;

        /**
         * @brief Creates a completed command result.
         * @param code The exit code for the completed command.
         * @return A CommandResult with the specified exit code and Completed state.
         */
        static CommandResult completed(int32_t code = 0)
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
        static CommandResult running(int32_t code = 0)
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
        static CommandResult waitingForInput(int32_t code = 0)
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

    struct StreamBundle
    {
        struct InputHandle
        {
            IInputChannel *channel = nullptr;

            InputHandle() = default;
            InputHandle(IInputChannel *ptr) : channel(ptr) {}

            bool available() const { return channel != nullptr && channel->available(); }
            ETString readAll() const { return channel != nullptr ? channel->readAll() : ETString(); }
            IInputChannel *get() const { return channel; }
        };

        struct OutputHandle
        {
            IOutputChannel *channel = nullptr;

            OutputHandle() = default;
            OutputHandle(IOutputChannel *ptr) : channel(ptr) {}

            void print(const ETString &s) const
            {
                if (channel != nullptr)
                {
                    channel->print(s);
                }
            }

            IOutputChannel *get() const { return channel; }
        };

        std::unique_ptr<IInputChannel> ownedInput;
        std::unique_ptr<IOutputChannel> ownedOutput;
        std::unique_ptr<IOutputChannel> ownedError;

        InputHandle input;
        OutputHandle output;
        OutputHandle error;

        StreamBundle() = delete;

        StreamBundle(std::unique_ptr<IInputChannel> in, std::unique_ptr<IOutputChannel> out, std::unique_ptr<IOutputChannel> err)
            : ownedInput(std::move(in)), ownedOutput(std::move(out)), ownedError(std::move(err)), input(ownedInput.get()), output(ownedOutput.get()), error(ownedError.get())
        {
        }

        StreamBundle(IInputChannel &in, IOutputChannel &out, IOutputChannel &err)
            : input(&in), output(&out), error(&err)
        {
        }

        StreamBundle(const StreamBundle &other) = delete;
        StreamBundle &operator=(const StreamBundle &other) = delete;

        StreamBundle(StreamBundle &other)
            : ownedInput(nullptr), ownedOutput(nullptr), ownedError(nullptr), input(nullptr), output(nullptr), error(nullptr)
        {
            ownedInput.swap(other.ownedInput);
            ownedOutput.swap(other.ownedOutput);
            ownedError.swap(other.ownedError);
            input.channel = ownedInput.get();
            output.channel = ownedOutput.get();
            error.channel = ownedError.get();
            other.input.channel = other.ownedInput.get();
            other.output.channel = other.ownedOutput.get();
            other.error.channel = other.ownedError.get();
        }

        StreamBundle(StreamBundle &&other) noexcept
            : ownedInput(std::move(other.ownedInput)), ownedOutput(std::move(other.ownedOutput)), ownedError(std::move(other.ownedError)), input(other.input), output(other.output), error(other.error)
        {
            input.channel = ownedInput ? ownedInput.get() : input.channel;
            output.channel = ownedOutput ? ownedOutput.get() : output.channel;
            error.channel = ownedError ? ownedError.get() : error.channel;
        }

        StreamBundle &operator=(StreamBundle &&other) noexcept
        {
            if (this != &other)
            {
                ownedInput = std::move(other.ownedInput);
                ownedOutput = std::move(other.ownedOutput);
                ownedError = std::move(other.ownedError);
                input = other.input;
                output = other.output;
                error = other.error;
                input.channel = ownedInput ? ownedInput.get() : input.channel;
                output.channel = ownedOutput ? ownedOutput.get() : output.channel;
                error.channel = ownedError ? ownedError.get() : error.channel;
            }
            return *this;
        }
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
        int32_t lastExitCode = 0;

        /**
         *  @brief Whether the command is running in interactive mode
         * */
        bool interactive = false;

        CommandContext(ETMap<ETString, ETString> &vars, int32_t exitCode = 0, bool isInteractive = false)
            : variables(vars), lastExitCode(exitCode), interactive(isInteractive)
        {
        }
    };

    struct CommandInvocation
    {
        const ETString keyword;
        const ETVector<ETString> arguments;
        CommandContext context;
        StreamBundle streams;

        CommandInvocation(const ETString &kw, const ETVector<ETString> &args, const CommandContext &ctx, const StreamBundle &bundle)
            : keyword(kw), arguments(args), context(ctx), streams(*bundle.input.get(), *bundle.output.get(), *bundle.error.get())
        {
        }

        CommandInvocation(const ETString &kw, const ETVector<ETString> &args, const CommandContext &ctx, IInputChannel &input, IOutputChannel &output, IOutputChannel &error)
            : keyword(kw), arguments(args), context(ctx), streams(input, output, error)
        {
        }
    };
}

#endif // I_COMMAND_RUNTIME_H

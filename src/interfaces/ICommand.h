#ifndef ICOMMAND_H
#define ICOMMAND_H

#include "ETTypes.h"
#include "IAutoCompleter.h"
#include "ICommandRuntime.h"
#include "OptionParser.h"

#ifdef execute
#undef execute
#endif

namespace EmbeddedTerminal
{
    /**
     * @brief Interface for terminal commands.
     *
     * Defines the contract for command execution and metadata retrieval.
     */
    class ICommand : public IAutoCompleter
    {
    public:
        virtual ~ICommand() = default;
        virtual ETString usage(const ETString &keyword) const = 0;
        virtual void onInterrupt()
        {
        }

        /**
         * @brief Invokes the command with the given arguments. Each command is called once per execution, and can return a CommandResult indicating whether it has completed, is still running, or is waiting for input. If a command returns Running or WaitingForInput, it may later be resumed by calling the resume() method with the same CommandInvocation context.
         * @param invocation The context for this command execution, including keyword, arguments, and I/O channels.
         * @param args Arguments for the command.
         * @return Execution result or status code.
         */
        virtual CommandResult invoke(CommandInvocation &invocation) = 0;

        /**
         * @brief Resumes a previously invoked command that returned Running or WaitingForInput. By default, commands do not support resuming, so this method just returns Completed. Commands that want to support resuming can override this method to implement their resume logic.
         * @param invocation The context for this command execution, including keyword, arguments, and I/O channels.
         * @return Execution result or status code.
         */
        virtual CommandResult resume(CommandInvocation &invocation) { return CommandResult::completed(0); };

        /// @brief Default implementation return s no suggestions
        /// Commands can override this to provide auto completion
        virtual ETVector<ETString> getSuggestions(const ETString &partial) const override
        {
            return ETVector<ETString>();
        }

    protected:
        // virtual ETString trigger(const ETString &keyword, const ETString &additional) { return ""; };
        friend class Terminal;
    };
} // namespace EmbeddedTerminal
#endif // ICOMMAND_H
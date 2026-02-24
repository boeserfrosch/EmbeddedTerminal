#ifndef ICOMMAND_H
#define ICOMMAND_H

#include "ETTypes.h"
#include "IAutoCompleter.h"
#include "ICommandRuntime.h"
#include "OptionParser.h"

namespace EmbeddedTerminal
{
    class ICommand : public IAutoCompleter
    {
    public:
        virtual ~ICommand() = default;
        virtual ETString usage(const ETString &keyword) = 0;

        virtual CommandResult execute(CommandInvocation &invocation)
        {
            ETString response = trigger(invocation.keyword, invocation.arguments);
            if (!response.empty())
            {
                invocation.stdoutChannel.print(response);
            }

            return CommandResult::completed(0);
        }

        /// @brief Default implementation returns no suggestions
        /// Commands can override this to provide auto completion
        virtual ETVector<ETString> getSuggestions(const ETString &partial) override
        {
            return ETVector<ETString>();
        }

    protected:
        virtual ETString trigger(const ETString &keyword, const ETString &additional) = 0;
        friend class Terminal;
    };
} // namespace EmbeddedTerminal
#endif // ICOMMAND_H
#ifndef HELP_H
#define HELP_H

#include "interfaces/IExecutionContext.h"
#include "interfaces/IAutoCompleter.h"
#include "DefaultAutoCompleters.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {

        class help : public ICommand
        {

        public:
            enum ErrorCode
            {
                None = 0,
                InvalidArgument = 1,
                Missused
            };

            help(IExecutionContext &terminal) : terminal_(terminal)
            {
            }
            ETString usage(const ETString &keyword) const override;

            CommandResult invoke(CommandInvocation &invocation) override;

            // Auto completion - suggest command names
            ETVector<ETString> getSuggestions(const ETString &partial) const override;

        private:
            ETString buildHelpOutput(const ETVector<ETString> &additional);
            IExecutionContext &terminal_;
        };
    };
};
#endif // HELP_H

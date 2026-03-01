#ifndef HELP_H
#define HELP_H

#include "Terminal.h"
#include "interfaces/IAutoCompleter.h"
#include "DefaultAutoCompleters.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {

        class help : public ICommand
        {

        public:
            help(Terminal &terminal) : terminal_(terminal)
            {
            }
            ETString usage(const ETString &keyword);

            CommandResult execute(CommandInvocation &invocation) override;

            ETString trigger(const ETString &keyword, const ETString &additional) override;

            // Auto completion - suggest command names
            ETVector<ETString> getSuggestions(const ETString &partial) override;

        private:
            ETString buildHelpOutput(const ETString &additional);
            Terminal& terminal_;
        };
    };
};
#endif // HELP_H

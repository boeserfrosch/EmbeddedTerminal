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
            help(Terminal &terminal) : _terminal(terminal)
            {
            }
            ETString usage(const ETString &keyword);

            ETString trigger(const ETString &keyword, const ETString &additional) override;

            // Auto completion - suggest command names
            ETVector<ETString> getSuggestions(const ETString &partial) override;

        private:
            Terminal &_terminal;
        };
    };
};
#endif // HELP_H

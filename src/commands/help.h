#ifndef HELP_H
#define HELP_H

#include "Terminal.h"

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

        private:
            Terminal &_terminal;
        };
    };
};
#endif // HELP_H

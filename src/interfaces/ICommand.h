#ifndef ICOMMAND_H
#define ICOMMAND_H

#include "ETTypes.h"

namespace EmbeddedTerminal
{
    class ICommand
    {
    public:
        virtual ETString usage(ETString &keyword) = 0;

    protected:
        virtual ETString trigger(ETString &keyword, ETString &additional) = 0;
        friend class Terminal;
    };
} // namespace EmbeddedTerminal
#endif // ICOMMAND_H
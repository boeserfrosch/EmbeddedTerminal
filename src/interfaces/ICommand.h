#ifndef ICOMMAND_H
#define ICOMMAND_H

#include "ETTypes.h"

namespace EmbeddedTerminal
{
    class ICommand
    {
    public:
        virtual ~ICommand() = default;
        virtual ETString usage(const ETString &keyword) = 0;

    protected:
        virtual ETString trigger(const ETString &keyword, const ETString &additional) = 0;
        friend class Terminal;
    };
} // namespace EmbeddedTerminal
#endif // ICOMMAND_H
#ifndef ECHO_H
#define ECHO_H

#include "interfaces/ICommand.h"
#include "DefaultAutoCompleters.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class echo : public ICommand
        {
        public:
            ETString usage(const ETString &keyword) const override;
            CommandResult invoke(CommandInvocation &invocation) override;
        };
    };
};

#endif // ECHO_H

#ifndef PWD_H
#define PWD_H

#include "DirectoryNavigator.h"
#include "interfaces/ICommand.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {

        class pwd : public ICommand
        {

        public:
            pwd(DirectoryNavigator &dir) : dir_(dir)
            {
            }
            ETString usage(const ETString &keyword) const override;

            CommandResult invoke(CommandInvocation &invocation) override;

        private:
            DirectoryNavigator &dir_;
        };
    };
};

#endif // PWD_H

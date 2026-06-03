#ifndef TOUCH_H
#define TOUCH_H

#include "DirectoryNavigator.h"
#include "interfaces/ICommand.h"
#include "DefaultAutoCompleters.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class touch : public ICommand
        {
        public:
            touch(DirectoryNavigator &dir) : dir_(dir), completer_(dir)
            {
            }

            ETString usage(const ETString &keyword) const override;
            ETVector<ETString> getSuggestions(const ETString &partial) const override;
            CommandResult invoke(CommandInvocation &invocation) override;

        private:
            DirectoryNavigator dir_;
            FilePathCompleter completer_;
        };
    };
};

#endif // TOUCH_H

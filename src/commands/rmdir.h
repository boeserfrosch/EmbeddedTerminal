#ifndef RMDIR_H
#define RMDIR_H

#include "DirectoryNavigator.h"
#include "interfaces/IAutoCompleter.h"
#include "DefaultAutoCompleters.h"
#include "interfaces/ICommand.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {

        class rmdir : public ICommand
        {

        public:
            enum ErrorCode
            {
                None = 0,
                Missused = 1,
                InvalidArgument = 2
            };

            rmdir(DirectoryNavigator &dir) : dir_(dir), completer_(dir)
            {
            }
            ETString usage(const ETString &keyword) const override;
            CommandResult invoke(CommandInvocation &invocation) override;

            // Auto completion - suggest directories to remove
            ETVector<ETString> getSuggestions(const ETString &partial) const override;

        private:
            DirectoryNavigator dir_;
            DirectoryCompleter completer_;
        };
    };
};
#endif // RMDIR_H

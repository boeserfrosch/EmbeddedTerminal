#ifndef CD_H
#define CD_H

#include "DirectoryNavigator.h"
#include "interfaces/ICommand.h"
#include "interfaces/IAutoCompleter.h"
#include "DefaultAutoCompleters.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {

        class cd : public ICommand
        {
        public:
            enum ErrorCode
            {
                None = 0,
                PathDoesNotExist = 1,
                PathNotDirectory = 2,
                FailedToChangeDirectory = 3
            };

            cd(DirectoryNavigator &dir) : dir_(dir), completer_(dir)
            {
            }
            ETString usage(const ETString &keyword) const override;

            CommandResult invoke(CommandInvocation &invocation) override;

            // Auto completion - suggest directories only
            ETVector<ETString> getSuggestions(const ETString &partial) const override;

        private:
            DirectoryNavigator &dir_;
            DirectoryCompleter completer_;
        };
    };
};

#endif // CD_H

#ifndef MKDIR_H
#define MKDIR_H

#include "DirectoryNavigator.h"
#include "interfaces/ICommand.h"
#include "interfaces/IAutoCompleter.h"
#include "DefaultAutoCompleters.h"
namespace EmbeddedTerminal
{
    namespace cmd
    {

        class mkdir : public ICommand
        {

        public:
            enum ErrorCode
            {
                NONE = 0,
                Missused = 1,
                FolderAlreadyExists = 2,
                FailedToCreateFolder = 3
            };
            mkdir(DirectoryNavigator &dir) : dir_(dir), completer_(dir)
            {
            }
            ETString usage(const ETString &keyword) const override;
            CommandResult invoke(CommandInvocation &invocation) override;

            // Auto completion - suggest directory paths for parent directory
            ETVector<ETString> getSuggestions(const ETString &partial) const override;

        private:
            DirectoryNavigator dir_;
            DirectoryCompleter completer_;
        };
    };
};
#endif // MKDIR_H

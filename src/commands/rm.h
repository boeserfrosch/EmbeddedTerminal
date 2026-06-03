#ifndef RM_H
#define RM_H

#include "DirectoryNavigator.h"
#include "interfaces/ICommand.h"
#include "interfaces/IAutoCompleter.h"
#include "DefaultAutoCompleters.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class rm : public ICommand
        {

        public:
            enum ErrorCode
            {
                None = 0,
                Missused = 1,
                InvalidArgument = 2
            };
            rm(DirectoryNavigator &dir) : dir_(dir), completer_(dir)
            {
            }
            ETString usage(const ETString &keyword) const override;
            CommandResult invoke(CommandInvocation &invocation) override;

            // Auto completion - suggest file paths for removal
            ETVector<ETString> getSuggestions(const ETString &partial) const override;

        private:
            DirectoryNavigator dir_;
            FilePathCompleter completer_;
        };
    };
};
#endif // RM_H

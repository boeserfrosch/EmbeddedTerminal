#ifndef CD_H
#define CD_H

#include "DirectoryNavigator.h"
#include "Terminal.h"
#include "interfaces/IAutoCompleter.h"
#include "DefaultAutoCompleters.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {

        enum CDerror
        {
            None = 0,
            PathDoesNotExist = 1,
            PathNotDirectory = 2,
            FailedToChangeDirectory = 3
        };

        class cd : public ICommand
        {

        public:
            cd(DirectoryNavigator &dir) : dir_(dir), completer_(dir)
            {
            }
            ETString usage(const ETString &keyword);

            CommandResult execute(CommandInvocation &invocation) override;
            ETString trigger(const ETString &keyword, const ETString &additional) override;

            // Auto completion - suggest directories only
            ETVector<ETString> getSuggestions(const ETString &partial) override;

        private:
            CDerror checkPath_(const ETString &path);
            CDerror changeDirectory_(const ETString &path);

            DirectoryNavigator &dir_;
            DirectoryCompleter completer_;
        };
    };
};

#endif // CD_H

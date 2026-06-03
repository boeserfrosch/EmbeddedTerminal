#ifndef WC_H
#define WC_H

#include "DirectoryNavigator.h"
#include "interfaces/ICommand.h"
#include "DefaultAutoCompleters.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class wc : public ICommand
        {
        public:
            wc(DirectoryNavigator &dir) : dir_(dir), completer_(dir)
            {
            }

            ETString usage(const ETString &keyword) const override;
            CommandResult invoke(CommandInvocation &invocation) override;
            ETVector<ETString> getSuggestions(const ETString &partial) const override;

        private:
            struct Counts
            {
                size_t lines = 0;
                size_t words = 0;
                size_t bytes = 0;
            };

            CommandResult countWordsInStdin_(CommandInvocation &invocation);
            CommandResult countWordsInFile_(const ETString &path, CommandInvocation &invocation, Counts &counts);
            Counts countText_(const ETString &text) const;

            DirectoryNavigator dir_;
            FilePathCompleter completer_;
        };
    };
};

#endif // WC_H

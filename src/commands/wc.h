#ifndef WC_H
#define WC_H

#include "DirectoryNavigator.h"
#include "DefaultAutoCompleters.h"
#include "Terminal.h"

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

            ETString usage(const ETString &keyword) override;
            ETString trigger(const ETString &keyword, const ETString &additional) override;
            CommandResult execute(CommandInvocation &invocation) override;
            ETVector<ETString> getSuggestions(const ETString &partial) override;

        private:
            struct Counts
            {
                size_t lines = 0;
                size_t words = 0;
                size_t bytes = 0;
            };

            Counts countText_(const ETString &text) const;
            ETString formatCounts_(const Counts &counts, const ETString &path = "") const;

            DirectoryNavigator dir_;
            FilePathCompleter completer_;
        };
    };
};

#endif // WC_H

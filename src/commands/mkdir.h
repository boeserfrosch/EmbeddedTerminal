#ifndef MKDIR_H
#define MKDIR_H

#include "DirectoryNavigator.h"
#include "interfaces/IAutoCompleter.h"
#include "DefaultAutoCompleters.h"
#include "Terminal.h"
namespace EmbeddedTerminal
{
    namespace cmd
    {

        class mkdir : public ICommand
        {

        public:
            mkdir(DirectoryNavigator &dir) : dir_(dir), completer_(dir)
            {
            }
            ETString usage(const ETString &keyword);

            ETString trigger(const ETString &keyword, const ETString &additional) override;

            // Auto completion - suggest directory paths for parent directory
            ETVector<ETString> getSuggestions(const ETString &partial) override;

        private:
            DirectoryNavigator dir_;
            DirectoryCompleter completer_;
        };
    };
};
#endif // MKDIR_H

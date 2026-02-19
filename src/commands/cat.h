#ifndef CAT_H
#define CAT_H

#include "DirectoryNavigator.h"
#include "interfaces/ICommand.h"
#include "interfaces/IAutoCompleter.h"
#include "DefaultAutoCompleters.h"
#include "Terminal.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class cat : public ICommand
        {

        public:
            cat(DirectoryNavigator &dir) : _dir(dir), _completer(dir)
            {
            }
            ETString usage(const ETString &keyword);
            ETString trigger(const ETString &keyword, const ETString &additional) override;

            // Auto completion - suggest files and directories
            ETVector<ETString> getSuggestions(const ETString &partial) override;

        private:
            EmbeddedTerminal::DirectoryNavigator &_dir;
            FilePathCompleter _completer;
        };
    };
};
#endif // CAT_H

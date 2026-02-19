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

        class cd : public ICommand
        {

        public:
            cd(DirectoryNavigator &dir) : _dir(dir), _completer(dir)
            {
            }
            ETString usage(const ETString &keyword);

            ETString trigger(const ETString &keyword, const ETString &additional) override;

            // Auto completion - suggest directories only
            ETVector<ETString> getSuggestions(const ETString &partial) override;

        private:
            DirectoryNavigator &_dir;
            DirectoryCompleter _completer;
        };
    };
};

#endif // CD_H

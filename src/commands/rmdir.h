#ifndef RMDIR_H
#define RMDIR_H

#include "DirectoryNavigator.h"
#include "interfaces/IAutoCompleter.h"
#include "DefaultAutoCompleters.h"
#include "Terminal.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {

        class rmdir : public ICommand
        {

        public:
            rmdir(DirectoryNavigator &dir) : _dir(dir), _completer(dir)
            {
            }
            ETString usage(const ETString &keyword);
            ETString trigger(const ETString &keyword, const ETString &additional) override;

            // Auto completion - suggest directories to remove
            ETVector<ETString> getSuggestions(const ETString &partial) override;

        private:
            DirectoryNavigator &_dir;
            DirectoryCompleter _completer;
        };
    };
};
#endif // RMDIR_H

#ifndef TAIL_H
#define TAIL_H

#include "DirectoryNavigator.h"
#include "interfaces/IAutoCompleter.h"
#include "DefaultAutoCompleters.h"
#include "Terminal.h"
namespace EmbeddedTerminal
{

    namespace cmd
    {
        class tail : public ICommand
        {

        public:
            tail(DirectoryNavigator &dir) : _dir(dir), _completer(dir)
            {
            }
            ETString usage(const ETString &keyword);
            ETString trigger(const ETString &keyword, const ETString &additional) override;

            // Auto completion - suggest file paths to tail
            ETVector<ETString> getSuggestions(const ETString &partial) override;

        private:
            DirectoryNavigator &_dir;
            FilePathCompleter _completer;
        };
    };
};
#endif // TAIL_H

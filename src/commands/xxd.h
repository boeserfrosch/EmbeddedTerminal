#ifndef XXD_H
#define XXD_H

#include "DirectoryNavigator.h"
#include "interfaces/ICommand.h"
#include "interfaces/IAutoCompleter.h"
#include "DefaultAutoCompleters.h"
#include "Terminal.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class xxd : public ICommand
        {

        public:
            xxd(DirectoryNavigator &dir) : _dir(dir), _completer(dir)
            {
            }
            ETString usage(const ETString &keyword);
            ETString trigger(const ETString &keyword, const ETString &additional) override;

            // Auto completion - suggest files and directories
            ETVector<ETString> getSuggestions(const ETString &partial) override;

        private:
            ETString generateHexDump(const char *content, size_t length, size_t startOffset = 0, size_t bytesPerLine = 16);

            EmbeddedTerminal::DirectoryNavigator &_dir;
            FilePathCompleter _completer;
        };
    };
};
#endif // XXD_H

#ifndef RM_H
#define RM_H

#include "DirectoryNavigator.h"
#include "interfaces/IAutoCompleter.h"
#include "DefaultAutoCompleters.h"
#include "Terminal.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class rm : public ICommand
        {

        public:
            rm(DirectoryNavigator &dir) : _dir(dir), _completer(dir)
            {
            }
            ETString usage(const ETString &keyword);
            ETString trigger(const ETString &keyword, const ETString &additional) override;

            // Auto completion - suggest file paths for removal
            ETVector<ETString> getSuggestions(const ETString &partial) override;

        private:
            DirectoryNavigator &_dir;
            FilePathCompleter _completer;
        };
    };
};
#endif // RM_H

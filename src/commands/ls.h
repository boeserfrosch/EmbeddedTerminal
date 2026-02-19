#ifndef LS_H
#define LS_H

#include "DirectoryNavigator.h"
#include "Terminal.h"
#include "interfaces/IAutoCompleter.h"
#include "DefaultAutoCompleters.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class ls : public ICommand
        {

        public:
            ls(DirectoryNavigator &dir) : _dir(dir), _completer(dir)
            {
            }
            ETString usage(const ETString &keyword);

            ETString trigger(const ETString &keyword, const ETString &additional) override;

            // Auto completion - suggest directories only
            ETVector<ETString> getSuggestions(const ETString &partial) override;

        private:
            struct
            {
                bool longListing = false;
            } lsConfig;
            void parseConf(std::vector<ETString> params);

        private:
            DirectoryNavigator &_dir;
            DirectoryCompleter _completer;
        };
    };
};
#endif // LS_H

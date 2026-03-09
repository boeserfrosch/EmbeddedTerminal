#ifndef TOUCH_H
#define TOUCH_H

#include "DirectoryNavigator.h"
#include "DefaultAutoCompleters.h"
#include "Terminal.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class touch : public ICommand
        {
        public:
            touch(DirectoryNavigator &dir) : dir_(dir), completer_(dir)
            {
            }

            ETString usage(const ETString &keyword) override;
            ETString trigger(const ETString &keyword, const ETString &additional) override;
            ETVector<ETString> getSuggestions(const ETString &partial) override;

        private:
            DirectoryNavigator dir_;
            FilePathCompleter completer_;
        };
    };
};

#endif // TOUCH_H

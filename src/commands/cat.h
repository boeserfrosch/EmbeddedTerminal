#ifndef CAT_H
#define CAT_H

#include "DirectoryNavigator.h"
#include "interfaces/ICommand.h"
#include "Terminal.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class cat : public ICommand
        {

        public:
            cat(DirectoryNavigator &dir) : _dir(dir)
            {
            }
            ETString usage(ETString &keyword);
            ETString trigger(ETString &keyword, ETString &additional) override;

        private:
            EmbeddedTerminal::DirectoryNavigator &_dir;
        };
    };
};
#endif // CAT_H

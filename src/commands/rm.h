#ifndef RM_H
#define RM_H

#include "DirectoryNavigator.h"
#include "Terminal.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class rm : public ICommand
        {

        public:
            rm(DirectoryNavigator &dir) : _dir(dir)
            {
            }
            ETString usage(ETString &keyword);
            ETString trigger(ETString &keyword, ETString &additional) override;

        private:
            DirectoryNavigator &_dir;
        };
    };
};
#endif // RM_H

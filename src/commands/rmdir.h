#ifndef RMDIR_H
#define RMDIR_H

#include "DirectoryNavigator.h"
#include "Terminal.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {

        class rmdir : public ICommand
        {

        public:
            rmdir(DirectoryNavigator &dir) : _dir(dir)
            {
            }
            ETString usage(const ETString &keyword);
            ETString trigger(const ETString &keyword, const ETString &additional) override;

        private:
            DirectoryNavigator &_dir;
        };
    };
};
#endif // RMDIR_H

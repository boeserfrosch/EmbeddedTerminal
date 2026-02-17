#ifndef CD_H
#define CD_H

#include "DirectoryNavigator.h"
#include "Terminal.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {

        class cd : public ICommand
        {

        public:
            cd(DirectoryNavigator &dir) : _dir(dir)
            {
            }
            ETString usage(const ETString &keyword);

            ETString trigger(const ETString &keyword, const ETString &additional) override;

        private:
            DirectoryNavigator &_dir;
        };
    };
};

#endif // CD_H

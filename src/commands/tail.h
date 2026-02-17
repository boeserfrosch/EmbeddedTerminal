#ifndef TAIL_H
#define TAIL_H

#include "DirectoryNavigator.h"
#include "Terminal.h"
namespace EmbeddedTerminal
{

    namespace cmd
    {
        class tail : public ICommand
        {

        public:
            tail(DirectoryNavigator &dir) : _dir(dir)
            {
            }
            ETString usage(const ETString &keyword);
            ETString trigger(const ETString &keyword, const ETString &additional) override;

        private:
            DirectoryNavigator &_dir;
        };
    };
};
#endif // TAIL_H

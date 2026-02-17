#ifndef MKDIR_H
#define MKDIR_H

#include "DirectoryNavigator.h"
#include "Terminal.h"
namespace EmbeddedTerminal
{
    namespace cmd
    {

        class mkdir : public ICommand
        {

        public:
            mkdir(DirectoryNavigator &dir) : _dir(dir)
            {
            }
            ETString usage(const ETString &keyword);

            ETString trigger(const ETString &keyword, const ETString &additional) override;

        private:
            DirectoryNavigator &_dir;
        };
    };
};
#endif // MKDIR_H

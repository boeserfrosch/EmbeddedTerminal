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
            ETString usage(const ETString &keyword);
            ETString trigger(const ETString &keyword, const ETString &additional) override;

        private:
            DirectoryNavigator &_dir;
        };
    };
};
#endif // RM_H

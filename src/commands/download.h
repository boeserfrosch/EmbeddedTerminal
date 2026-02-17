#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include "DirectoryNavigator.h"
#include "Terminal.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class download : public ICommand
        {

        public:
            download(DirectoryNavigator &dir) : _dir(dir)
            {
            }
            ETString usage(ETString &keyword);

        protected:
            ETString trigger(ETString &keyword, ETString &additional) override;

        private:
        private:
            DirectoryNavigator &_dir;
        };
    };
};
#endif // DOWNLOAD_H

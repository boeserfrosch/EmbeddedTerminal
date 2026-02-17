#ifndef LS_H
#define LS_H

#include "DirectoryNavigator.h"
#include "Terminal.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class ls : public ICommand
        {

        public:
            ls(DirectoryNavigator &dir) : _dir(dir)
            {
            }
            ETString usage(const ETString &keyword);

            ETString trigger(const ETString &keyword, const ETString &additional) override;

        private:
            struct
            {
                bool longListing = false;
            } lsConfig;
            void parseConf(std::vector<ETString> params);

        private:
            DirectoryNavigator &_dir;
        };
    };
};
#endif // LS_H

#ifndef DF_H
#define DF_H

#include "Terminal.h"
#include "interfaces/ICommand.h"
#include "interfaces/IFileSystem.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {

        class df : public ICommand
        {

        public:
            df(IFileSystem &card) : _card(card)
            {
            }
            ETString usage(const ETString &keyword);
            ETString trigger(const ETString &keyword, const ETString &additional) override;

        private:
            IFileSystem &_card;
        };
    };
};
#endif // DF_H

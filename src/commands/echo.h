#ifndef ECHO_H
#define ECHO_H

#include "Terminal.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class echo : public ICommand
        {
        public:
            ETString usage(const ETString &keyword) override;
            ETString trigger(const ETString &keyword, const ETString &additional) override;
        };
    };
};

#endif // ECHO_H

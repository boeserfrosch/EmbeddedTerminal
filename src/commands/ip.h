#ifndef IP_H
#define IP_H

#include "interfaces/INetworkInterface.h"
#include "Terminal.h"
namespace EmbeddedTerminal
{
    namespace cmd
    {
        class ip : public ICommand
        {

        public:
            ip(INetworkInterface &network) : _net(network)
            {
            }
            ETString usage(ETString &keyword);
            ETString trigger(ETString &keyword, ETString &additional) override;

        private:
            INetworkInterface &_net;
        };
    };
};
#endif // CAT_H

#ifndef PING_H
#define PING_H

#include "interfaces/INetworkInterface.h"
#include "Terminal.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class ping : public ICommand
        {
        public:
            ping(INetworkInterface &network) : _net(network)
            {
            }
            ETString usage(const ETString &keyword) override;

        protected:
            ETString trigger(const ETString &keyword, const ETString &additional) override;

        private:
            INetworkInterface &_net;
        };
    } // namespace cmd
} // namespace EmbeddedTerminal

#endif // PING_H

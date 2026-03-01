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
            ping(INetworkSystem &network) : net_(network)
            {
            }
            ETString usage(const ETString &keyword) override;
            ETString trigger(const ETString &keyword, const ETString &additional) override;

        protected:
        private:
            INetworkSystem& net_;
        };
    } // namespace cmd
} // namespace EmbeddedTerminal

#endif // PING_H

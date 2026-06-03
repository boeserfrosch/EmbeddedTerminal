#ifndef PING_H
#define PING_H

#include "interfaces/INetworkInterface.h"
#include "interfaces/ICommand.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class ping : public ICommand
        {
        public:
            enum ErrorCode
            {
                None = 0,
                Missused = 1,
            };

            ping(INetworkSystem &network) : net_(network)
            {
            }
            ETString usage(const ETString &keyword) const override;
            CommandResult invoke(CommandInvocation &invocation) override;

        protected:
        private:
            INetworkSystem &net_;
        };
    } // namespace cmd
} // namespace EmbeddedTerminal

#endif // PING_H

#ifndef IP_H
#define IP_H

#include "interfaces/ICommand.h"
#include "interfaces/INetworkInterface.h"
namespace EmbeddedTerminal
{
    namespace cmd
    {
        class ip : public ICommand
        {

        public:
            enum ErrorCode
            {
                NONE = 0,
                INVALID_INTERFACE = 1,
                NO_INTERFACES = 2,
                MISSUSED,
            };

            /**
             * @brief Constructor for the ip command.
             * @param networks Reference to the INetworkSystem to query for network interfaces.
             */
            ip(INetworkSystem &networks) : net_(networks)
            {
            }

            ETString usage(const ETString &keyword) const override;
            // ETString trigger(const ETString &keyword, const ETString &additional) override;
            CommandResult invoke(CommandInvocation &invocation) override;

        private:
            INetworkSystem &net_;
        };
    };
};
#endif // IP_H

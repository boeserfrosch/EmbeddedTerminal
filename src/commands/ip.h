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
            /**
             * @brief Constructor for the ip command.
             * @param networks Reference to the INetworkSystem to query for network interfaces.
             */
            ip(INetworkSystem &networks) : net_(networks)
            {
            }

            ETString usage(const ETString &keyword);
            ETString trigger(const ETString &keyword, const ETString &additional) override;

        private:
            INetworkSystem& net_;
        };
    };
};
#endif // IP_H

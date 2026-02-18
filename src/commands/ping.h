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

            // Platform-specific ping implementations
#if defined(ESP32)
            ETString _pingESP32(const ETString &target);
#endif
#if defined(ESP8266)
            ETString _pingESP8266(const ETString &target);
#endif
#if defined(__linux__) || defined(__APPLE__) || defined(_WIN32)
            ETString _pingNative(const ETString &target);
#endif
        };
    } // namespace cmd
} // namespace EmbeddedTerminal

#endif // PING_H

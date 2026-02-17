#ifndef INETWORKINTERFACE_H
#define INETWORKINTERFACE_H

#include "../ETTypes.h"

namespace EmbeddedTerminal
{

    struct NetworkInfo
    {
        ETString name;
        ETString ip;
        ETString mac;
        ETString netmask;
        ETString gateway;
        bool isUp = false;
    };

    class INetworkInterface
    {
    public:
        virtual ~INetworkInterface() {}

        // Get all network interfaces
        virtual ETMap<ETString, NetworkInfo> getInterfaces() const = 0;

        // Get info for a specific interface by name
        virtual NetworkInfo getInterface(const ETString &name) const = 0;
    };

} // namespace EmbeddedTerminal

#endif // INETWORKINTERFACE_H

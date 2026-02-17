#ifndef MOCKNETWORKINTERFACE_H
#define MOCKNETWORKINTERFACE_H

#include "../../src/interfaces/INetworkInterface.h"
#include "../../src/ETTypes.h"

class MockNetworkInterface : public EmbeddedTerminal::INetworkInterface
{
public:
    ETMap<ETString, EmbeddedTerminal::NetworkInfo> interfaces;

    MockNetworkInterface() : interfaces()
    {
    }

    void addInterface(ETString name, ETString ip, ETString mac = "", ETString netmask = "", ETString gateway = "", bool isup = false)
    {
        EmbeddedTerminal::NetworkInfo iface;
        iface.name = name;
        iface.ip = ip;
        iface.mac = mac;
        iface.gateway = gateway;
        iface.netmask = netmask;
        iface.isUp = isup;
        interfaces[name.c_str()] = iface;
    }

    ETMap<ETString, EmbeddedTerminal::NetworkInfo> getInterfaces() const override
    {
        return interfaces;
    }

    EmbeddedTerminal::NetworkInfo getInterface(const ETString &name) const override
    {
        if (interfaces.find(name) == interfaces.end())
            return EmbeddedTerminal::NetworkInfo();
        auto it = interfaces.find(name);
        return it->second;
    }
};

#endif // MOCKNETWORKINTERFACE_H

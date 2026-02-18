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

    ETString ping(const ETString &target) override
    {
        // Mock implementation: simulate ping based on target
        // For testing purposes, return a simple response
        if (target.empty() || target == "10.255.255.255" || target == "invalid")
        {
            return "Host " + target + " is not reachable\n";
        }

        // Simulate successful ping with mock statistics
        ETString result = target + " pinged 3 times:\n";
        result += "  Average time: 25 ms\n";
        result += "  Min time: 20 ms\n";
        result += "  Max time: 30 ms\n";

        return result;
    }
};

#endif // MOCKNETWORKINTERFACE_H

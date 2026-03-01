#ifndef MOCKNETWORKINTERFACE_H
#define MOCKNETWORKINTERFACE_H

#include "../../src/interfaces/INetworkInterface.h"
#include "../../src/ETTypes.h"

class MockNetworkInterface : public EmbeddedTerminal::INetworkInterface
{
public:
    EmbeddedTerminal::NetworkInfo info_;

    MockNetworkInterface()
    {
        info_.name = "MockInterface";
        info_.ip = "192.168.1.100";
        info_.mac = "00:11:22:33:44:55";
        info_.netmask = "255.255.255.0";
        info_.gateway = "192.168.1.1";
        info_.isUp = true;
    }

    MockNetworkInterface(EmbeddedTerminal::NetworkInfo info) : info_(info)
    {
    }

    EmbeddedTerminal::NetworkInfo info() const override
    {
        return info_;
    }

    ETString ping(const ETString &target) override
    {
        if (target.empty() || target == "invalid" || target == "10.255.255.255")
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

#include "ip.h"

using namespace EmbeddedTerminal::cmd;
ETString ip::trigger(ETString &keyword, ETString &additional)
{
    ETString result;
    auto interfaces = _net.getInterfaces();
    if (interfaces.empty())
    {
        return "No interface available\n";
    }

    additional = additional.trim();
    if (!additional.empty())
    {
        auto iface = _net.getInterface(additional);
        if (iface.name.empty())
        {
            return "Unknown interface\n";
        }
        result += iface.name + ": ";
        result += iface.ip + " ";
        result += iface.mac + " ";
        result += iface.netmask + " ";
        result += iface.gateway + " ";
        result += (iface.isUp ? "UP" : "DOWN");
        result += "\n";
        return result;
    }

    for (const auto &ifacePair : interfaces)
    {
        auto iface = ifacePair.second;
        result += iface.name + ": ";
        result += iface.ip + " ";
        result += iface.mac + " ";
        result += iface.netmask + " ";
        result += iface.gateway + " ";
        result += (iface.isUp ? "UP" : "DOWN");
        result += "\n";
    }
    return result;
}

ETString ip::usage(ETString &keyword)
{
    return keyword + " - Returns info for all network interfaces\n";
}

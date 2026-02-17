#include "ip.h"

using namespace EmbeddedTerminal::cmd;
ETString ip::trigger(const ETString &keyword, const ETString &additional)
{
    ETString result;
    auto interfaces = _net.getInterfaces();
    if (interfaces.empty())
    {
        return "No interface available\n";
    }

    ETString params = additional.trim();
    if (!params.empty())
    {
        auto iface = _net.getInterface(params);
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

ETString ip::usage(const ETString &keyword)
{
    return keyword + " - Returns info for all network interfaces\n";
}

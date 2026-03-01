#include "ip.h"

using namespace EmbeddedTerminal::cmd;
ETString ip::trigger(const ETString &keyword, const ETString &additional)
{
    ETString result;
    auto interfaces = net_.interfaces();
    if (interfaces.empty())
    {
        return "No interface available\n";
    }

    ETString params = additional.trim();
    if (!params.empty())
    {
        auto iface = net_.getInterface(params);
        if (!iface || iface->info().name.empty())
        {
            return "Unknown interface\n";
        }
        result += iface->info().name + ": ";
        result += iface->info().ip + " ";
        result += iface->info().mac + " ";
        result += iface->info().netmask + " ";
        result += iface->info().gateway + " ";
        result += (iface->info().isUp ? "UP" : "DOWN");
        result += "\n";
        return result;
    }

    for (const auto &iface : interfaces)
    {
        result += iface->info().name + ": ";
        result += iface->info().ip + " ";
        result += iface->info().mac + " ";
        result += iface->info().netmask + " ";
        result += iface->info().gateway + " ";
        result += (iface->info().isUp ? "UP" : "DOWN");
        result += "\n";
    }
    return result;
}

ETString ip::usage(const ETString &keyword)
{
    return keyword + " - Returns info for all network interfaces\n";
}

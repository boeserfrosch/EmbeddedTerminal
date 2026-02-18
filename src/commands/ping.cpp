#include "ping.h"

using namespace EmbeddedTerminal::cmd;

ETString ping::trigger(const ETString &keyword, const ETString &additional)
{
    ETString params = additional.trim();

    if (params.empty())
    {
        return "Cannot ping without a target\n";
    }

    // Extract the target (first argument)
    ETString target = params;
    size_t spacePos = params.find(' ');
    if (spacePos != ETString::npos)
    {
        target = params.substr(0, spacePos);
    }

    // Delegate to network interface implementation
    return _net.ping(target);
}
ETString ping::usage(const ETString &keyword)
{
    return keyword + " <host> - Ping a host (IP address or hostname)\n";
}

#include "ping.h"

// Platform-specific ping implementation
#if defined(ESP32)
#include <ESP32Ping.h>
#endif

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

#if defined(ESP32)
    // ESP32-specific implementation using ESP32Ping library
    const int pingCount = 3;
    bool pingable = Ping.ping(target.c_str(), pingCount);

    if (!pingable)
    {
        return "Host " + target + " is not reachable\n";
    }

    ETString result = target + " pinged " + ETString(pingCount) + " times:\n";
    result += "  Average time: " + ETString((int)Ping.averageTime()) + " ms\n";
    result += "  Min time: " + ETString((int)Ping.minTime()) + " ms\n";
    result += "  Max time: " + ETString((int)Ping.maxTime()) + " ms\n";
    
    return result;
#else
    // Fallback for non-ESP32 platforms
    return "Ping command is only available on ESP32 platform\n";
#endif
}

ETString ping::usage(const ETString &keyword)
{
    return keyword + " <host> - Ping a host (IP address or hostname)\n";
}

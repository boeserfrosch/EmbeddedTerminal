#include "ping.h"
#include <cstdlib>

// Platform-specific ping implementations
#if defined(ESP32)
#include <ESP32Ping.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
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
    return _pingESP32(target);
#elif defined(ESP8266)
    return _pingESP8266(target);
#elif defined(__linux__) || defined(__APPLE__) || defined(_WIN32)
    return _pingNative(target);
#else
    return "Ping command is not available on this platform\n";
#endif
}

#if defined(ESP32)
ETString ping::_pingESP32(const ETString &target)
{
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
}
#endif

#if defined(ESP8266)
ETString ping::_pingESP8266(const ETString &target)
{
    // ESP8266-specific implementation using built-in WiFi.ping()
    const int pingCount = 3;
    
    // WiFi.ping() returns the average time in milliseconds, or 0 if unreachable
    int avgTime = WiFi.ping(target.c_str());

    if (avgTime == 0)
    {
        return "Host " + target + " is not reachable\n";
    }

    ETString result = target + " pinged " + ETString(pingCount) + " times:\n";
    result += "  Average time: " + ETString(avgTime) + " ms\n";

    return result;
}
#endif

#if defined(__linux__) || defined(__APPLE__) || defined(_WIN32)
ETString ping::_pingNative(const ETString &target)
{
    // Native platform implementation using system ping command
    ETString command;
    
#ifdef _WIN32
    // Windows command: ping -n 3 target
    command = "ping -n 3 " + target + " > nul 2>&1";
#else
    // Linux/macOS command: ping -c 3 target
    command = "ping -c 3 " + target + " > /dev/null 2>&1";
#endif

    int result = system(command.c_str());

    if (result != 0)
    {
        return "Host " + target + " is not reachable\n";
    }

    // For native platforms, we return success message without detailed stats
    // (would require parsing ping output which is platform-dependent)
    ETString response = target + " is reachable\n";

    return response;
}
#endif

ETString ping::usage(const ETString &keyword)
{
    return keyword + " <host> - Ping a host (IP address or hostname)\n";
}

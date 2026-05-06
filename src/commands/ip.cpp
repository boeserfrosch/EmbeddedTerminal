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

    OptionParser parser;
    parser.addOption("-i", "--interface", "Show info for the specified interface", true);
    auto parseResult = parser.parse(additional);
    if (!parseResult.success)
    {
        return "ip error: " + parseResult.errorMessage + "\n" + usage(keyword);
    }

    // Collect interface names to display
    ETVector<ETString> interfaceNames;

    // Check if --interface option was specified
    auto it = parseResult.options.find("--interface");
    if (it != parseResult.options.end() && !it->second.empty())
    {
        // Add all interface names specified with -i/--interface
        for (const auto &name : it->second)
        {
            interfaceNames.push_back(name);
        }
    }

    // If no specific interfaces requested, show all
    if (interfaceNames.empty())
    {
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

    // Show specific interfaces
    for (const auto &name : interfaceNames)
    {
        auto iface = net_.getInterface(name);
        if (!iface || iface->info().name.empty())
        {
            result += "Unknown interface: " + name + "\n";
            continue;
        }
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
    return keyword + " - Returns info for all network interfaces\n" + keyword + " -i|--interface <interface> - Returns info for the specified interface\n";
}

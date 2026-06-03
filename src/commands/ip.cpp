#include "ip.h"

using namespace EmbeddedTerminal::cmd;

ETString ip::usage(const ETString &keyword) const
{
    return keyword + " - Returns info for all network interfaces\n" + keyword + " -i|--interface <interface> - Returns info for the specified interface\n";
}

EmbeddedTerminal::CommandResult ip::invoke(CommandInvocation &invocation)
{
    ETString result;
    auto interfaces = net_.interfaces();
    if (interfaces.empty())
    {
        invocation.streams.output.print("No interface available\n");
        return CommandResult::completed(NO_INTERFACES);
    }

    OptionParser parser;
    parser.addOption("-i", "--interface", "Show info for the specified interface", true);
    auto parseResult = parser.parse(invocation.arguments);
    if (!parseResult.success)
    {
        invocation.streams.output.print(usage(invocation.keyword));
        return CommandResult::completed(MISSUSED);
    }

    // Collect interface names to display
    ETVector<INetworkInterface *> selectedInterfaces;

    // Check if --interface option was specified
    auto it = parseResult.options.find("--interface");
    if (it != parseResult.options.end() && !it->second.empty())
    {
        // Add all interface names specified with -i/--interface
        for (const auto &name : it->second)
        {
            if (!net_.getInterface(name) || net_.getInterface(name)->info().name.empty())
            {
                invocation.streams.output.print("Unknown interface: " + name + "\n");
                continue;
            }
            selectedInterfaces.push_back(net_.getInterface(name));
        }
    }
    else
    {
        selectedInterfaces = interfaces;
    }

    for (const auto &iface : selectedInterfaces)
    {
        invocation.streams.output.print(
            iface->info().name + ": " +
            iface->info().ip + " " +
            iface->info().mac + " " +
            iface->info().netmask + " " +
            iface->info().gateway + " " +
            (iface->info().isUp ? "UP" : "DOWN") +
            "\n");
    }
    return CommandResult::completed(0);
}

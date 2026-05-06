#include "ping.h"

using namespace EmbeddedTerminal::cmd;

ETString ping::trigger(const ETString &keyword, const ETString &additional)
{
    OptionParser parser;
    parser.addRequiredRemainingArgument("target");
    auto parseResult = parser.parse(additional);

    if (!parseResult.success)
    {
        return parseResult.errorMessage + "\n" + usage(keyword);
    }

    // Extract the target (first argument)
    ETString target = parseResult.options["target"][0];

    // Delegate to network interface implementation
    return net_.ping(target);
}
ETString ping::usage(const ETString &keyword)
{
    return keyword + " <host> - Ping a host (IP address or hostname)\n";
}

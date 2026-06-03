#include "ping.h"

using namespace EmbeddedTerminal::cmd;

ETString ping::usage(const ETString &keyword) const
{
    return keyword + " <target> - Ping a host (IP address or hostname)\n";
}

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::ping::invoke(CommandInvocation &invocation)
{
    OptionParser parser;
    parser.addRequiredRemainingArgument("target");
    auto parseResult = parser.parse(invocation.arguments);

    if (!parseResult.success)
    {
        invocation.streams.output.print(usage(invocation.keyword));
        return CommandResult::completed(Missused);
    }

    // Extract the target (first argument)
    ETString target = parseResult.options["target"][0];

    // Delegate to network interface implementation
    invocation.streams.output.print(net_.ping(target));
    return CommandResult::completed(ErrorCode::None);
}

#include "commands/echo.h"
#include "echo.h"

namespace EmbeddedTerminal
{
    using namespace EmbeddedTerminal::cmd;

    ETString echo::usage(const ETString &keyword) const
    {
        return keyword + " <text> - Print the specified text\n";
    }

    CommandResult EmbeddedTerminal::cmd::echo::invoke(CommandInvocation &invocation)
    {
        invocation.streams.output.print(invocation.arguments.empty() ? "\n" : invocation.arguments[0] + "\n");
        return CommandResult::completed(0);
    }
} // namespace EmbeddedTerminal
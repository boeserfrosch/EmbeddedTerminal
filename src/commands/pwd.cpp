#include "commands/pwd.h"

using namespace EmbeddedTerminal::cmd;

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::pwd::invoke(CommandInvocation &invocation)
{
    invocation.streams.output.print(dir_.pwd() + "\n");
    return CommandResult::completed(0);
}

ETString pwd::usage(const ETString &keyword) const
{
    return keyword + " - Print the current working directory\n";
}

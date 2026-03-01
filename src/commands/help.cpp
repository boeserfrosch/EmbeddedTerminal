#include "help.h"

using namespace EmbeddedTerminal;

ETString cmd::help::buildHelpOutput(const ETString &additional)
{
    ETString params = additional.trim();
    auto parts = split(params, " ");

    ETString result = "Available commands\n";
    auto commands = terminal_.getCommands();

    if (params.empty())
    {
        for (const auto &command : commands)
        {
            result += command.first + "\n";
        }
        result += "\n";
        return result;
    }

    auto cmd = commands.find(parts[0]);
    if (cmd == commands.end())
    {
        return "Unknown command!\n";
    }
    ETString cmdKey = cmd->first;
    return cmd->second->usage(cmdKey);
}

CommandResult cmd::help::execute(CommandInvocation &invocation)
{
    ETString output = buildHelpOutput(invocation.arguments);
    if (!output.empty())
    {
        invocation.stdoutChannel.print(output);
    }

    return CommandResult::completed(0);
}

ETString cmd::help::trigger(const ETString &keyword, const ETString &additional)
{
    (void)keyword;
    return buildHelpOutput(additional);
}

ETString cmd::help::usage(const ETString &keyword)
{
    return keyword + " - Returns all available commands\n" +
           keyword + " [command] - Returns the usage for the specific command\n";
}

ETVector<ETString> cmd::help::getSuggestions(const ETString &partial)
{
    CommandCompleter completer(terminal_.getCommands());
    return completer.getSuggestions(partial);
}

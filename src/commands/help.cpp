#include "help.h"

using namespace EmbeddedTerminal;
ETString cmd::help::trigger(ETString &keyword, ETString &additional)
{
    additional = additional.trim();
    auto parts = split(additional, " ");

    ETString result = "Available commands\n";
    auto commands = _terminal.getCommands();

    if (additional.empty())
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

ETString cmd::help::usage(ETString &keyword)
{
    return keyword + " - Returns all available commands\n" +
           keyword + " [command] - Returns the usage for the specific command\n";
}

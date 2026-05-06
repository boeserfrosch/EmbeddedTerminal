#include "help.h"

using namespace EmbeddedTerminal;

ETString cmd::help::buildHelpOutput(const ETString &additional)
{
    OptionParser parser;
    parser.addOptionalRemainingArgument("command");
    auto parseResult = parser.parse(additional);
    if (!parseResult.success)
    {
        return "help error: " + parseResult.errorMessage + "\n" + usage("help");
    }

    auto commands = terminal_.getCommands();
    if (parseResult.options.find("command") != parseResult.options.end() && !parseResult.options["command"].empty())
    {
        ETString cmdName = parseResult.options["command"][0].trim();
        auto cmdIt = commands.find(cmdName);
        if (cmdIt == commands.end())
        {
            return "Unknown command: " + cmdName + "\n";
        }
        return cmdIt->second->usage(cmdIt->first);
    }

    ETString result = "Available commands\n";

    for (const auto &command : commands)
    {
        result += command.first + "\n";
    }
    result += "\n";
    return result;
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
           keyword + " <command> - Returns the usage for the specific command\n";
}

ETVector<ETString> cmd::help::getSuggestions(const ETString &partial)
{
    CommandCompleter completer(terminal_.getCommands());
    return completer.getSuggestions(partial);
}

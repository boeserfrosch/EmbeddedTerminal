#include "help.h"

using namespace EmbeddedTerminal;

CommandResult cmd::help::invoke(CommandInvocation &invocation)
{
    OptionParser parser;
    parser.addOptionalRemainingArgument("command");
    auto parseResult = parser.parse(invocation.arguments);
    if (!parseResult.success)
    {
        invocation.streams.output.print(usage(invocation.keyword));
        return CommandResult::completed(ErrorCode::Missused);
    }

    if (parseResult.options.find("command") != parseResult.options.end() && !parseResult.options["command"][0].empty())
    {
        ETString cmdName = parseResult.options["command"][0].trim();
        if (!terminal_.existsCommand(cmdName))
        {
            invocation.streams.output.print("Unknown command: '" + cmdName + "'\n");
            return CommandResult::completed(ErrorCode::InvalidArgument);
        }
        invocation.streams.output.print(terminal_.getCommand(cmdName)->usage(cmdName));
        return CommandResult::completed(ErrorCode::None);
    }

    invocation.streams.output.print("Available commands:\n");
    auto commands = terminal_.getCommands();
    for (const auto &command : commands)
    {
        invocation.streams.output.print(command.first + "\n");
    }
    invocation.streams.output.print("\n");
    return CommandResult::completed(ErrorCode::None);
}

ETString cmd::help::usage(const ETString &keyword) const
{
    return keyword + " - Returns all available commands\n" +
           keyword + " <command> - Returns the usage for the specific command\n";
}

ETVector<ETString> cmd::help::getSuggestions(const ETString &partial) const
{
    CommandCompleter completer(terminal_.getCommands());
    return completer.getSuggestions(partial);
}

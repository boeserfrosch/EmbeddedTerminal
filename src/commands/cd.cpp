#include "commands/cd.h"
#include "cd.h"

using namespace EmbeddedTerminal::cmd;

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::cd::invoke(CommandInvocation &invocation)
{
    OptionParser parser;
    parser.addRequiredRemainingArgument("path");

    auto parseResult = parser.parse(invocation.arguments);
    if (!parseResult.success)
    {
        invocation.streams.output.print(usage(invocation.keyword));
        return CommandResult::completed(1); // Error code for missing argument
    }

    ETString path = parseResult.options["path"][0];
    if (!dir_.exists(path.c_str()))
    {
        invocation.streams.error.print("Path does not exist: " + path);
        return CommandResult::completed(ErrorCode::PathDoesNotExist);
    }
    else if (!dir_.isDirectory(path.c_str()))
    {
        invocation.streams.error.print("Path is not a directory: " + path);
        return CommandResult::completed(ErrorCode::PathNotDirectory);
    }

    if (!dir_.cd(path.c_str()))
    {
        invocation.streams.error.print("Failed to change directory to: " + path);
        return CommandResult::completed(ErrorCode::FailedToChangeDirectory);
    }
    invocation.streams.output.print("> " + dir_.pwd() + "\n");
    return CommandResult::completed(ErrorCode::None); // Success
}

ETString cd::usage(const ETString &keyword) const
{
    return keyword + " <path> - Change the current directory relative to path\n";
}

ETVector<ETString> cd::getSuggestions(const ETString &partial) const
{
    // Delegate to DirectoryCompleter
    return completer_.getSuggestions(partial);
}
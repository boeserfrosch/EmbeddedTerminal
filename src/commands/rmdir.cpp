#include "rmdir.h"

using namespace EmbeddedTerminal::cmd;

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::rmdir::invoke(CommandInvocation &invocation)
{
    OptionParser parser;
    parser.addRequiredRemainingArgument("folder");
    auto parseResult = parser.parse(invocation.arguments);
    if (!parseResult.success)
    {
        invocation.streams.output.print(usage(invocation.keyword));
        return CommandResult::completed(ErrorCode::Missused);
    }

    ETString folderName = parseResult.options["folder"][0];

    if (!dir_.exists(folderName))
    {
        invocation.streams.output.print(folderName + " did not exists\n");
        return CommandResult::completed(ErrorCode::InvalidArgument);
    }
    if (!dir_.isDirectory(folderName))
    {
        invocation.streams.output.print(folderName + " is not a directory\n");
        return CommandResult::completed(ErrorCode::InvalidArgument);
    }
    if (!dir_.isEmpty(folderName.c_str()))
    {
        invocation.streams.output.print(folderName + " is not empty\n");
        return CommandResult::completed(ErrorCode::InvalidArgument);
    }
    if (!dir_.rmdir(folderName))
    {
        invocation.streams.output.print("Could not remove " + folderName + "\n");
        return CommandResult::completed(ErrorCode::InvalidArgument);
    }
    invocation.streams.output.print(folderName + " removed\n");
    return CommandResult::completed(ErrorCode::None);
}

ETString rmdir::usage(const ETString &keyword) const
{
    return keyword + " <folder> - Remove the specified folder\n";
}

ETVector<ETString> rmdir::getSuggestions(const ETString &partial) const
{
    return completer_.getSuggestions(partial);
}

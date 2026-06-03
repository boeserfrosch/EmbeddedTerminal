#include "mkdir.h"

#include <sstream>

using namespace EmbeddedTerminal::cmd;

EmbeddedTerminal::CommandResult mkdir::invoke(CommandInvocation &invocation)
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

    if (dir_.exists(folderName))
    {
        invocation.streams.output.print(folderName + " already exists\n");
        return CommandResult::completed(ErrorCode::FolderAlreadyExists);
    }

    auto result = dir_.mkdir(folderName.c_str());
    if (!result)
    {
        invocation.streams.output.print("Could not create " + folderName + "\n");
        return CommandResult::completed(ErrorCode::FailedToCreateFolder);
    }
    invocation.streams.output.print(folderName + " created\n");
    return CommandResult::completed(ErrorCode::NONE);
}

ETString mkdir::usage(const ETString &keyword) const
{
    return keyword + " <folder> - Create the specified folder in the current directory\n";
}

ETVector<ETString> mkdir::getSuggestions(const ETString &partial) const
{
    return completer_.getSuggestions(partial);
}

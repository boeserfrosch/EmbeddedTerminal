#include "rm.h"

using namespace EmbeddedTerminal::cmd;

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::rm::invoke(CommandInvocation &invocation)
{
    OptionParser parser;
    parser.addRequiredRemainingArgument("file");
    auto parseResult = parser.parse(invocation.arguments);
    if (!parseResult.success)
    {
        invocation.streams.output.print(usage(invocation.keyword));
        return CommandResult::completed(ErrorCode::Missused);
    }
    ETString fileName = parseResult.options["file"][0];

    if (!dir_.exists(fileName.c_str()))
    {
        invocation.streams.output.print(fileName + " did not exist!\n");
        return CommandResult::completed(ErrorCode::InvalidArgument);
    }
    if (dir_.isDirectory(fileName))
    {
        invocation.streams.output.print(fileName + "is not a file\n");
        return CommandResult::completed(ErrorCode::InvalidArgument);
    }
    auto result = dir_.remove(fileName.c_str());
    if (result)
    {
        invocation.streams.output.print(fileName + " removed\n");
        return CommandResult::completed(ErrorCode::None);
    }
    invocation.streams.output.print("Error on deleting\n");
    return CommandResult::completed(ErrorCode::InvalidArgument);
}

ETString rm::usage(const ETString &keyword) const
{
    return keyword + " <file> - Remove the specified file\n";
}

ETVector<ETString> rm::getSuggestions(const ETString &partial) const
{
    return completer_.getSuggestions(partial);
}

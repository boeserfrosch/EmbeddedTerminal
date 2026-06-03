#include "commands/touch.h"
#include "touch.h"

using namespace EmbeddedTerminal::cmd;

ETString touch::usage(const ETString &keyword) const
{
    return keyword + " [file] - Create file if missing, otherwise leave existing file unchanged\n";
}

ETVector<ETString> touch::getSuggestions(const ETString &partial) const
{
    return completer_.getSuggestions(partial);
}

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::touch::invoke(CommandInvocation &invocation)
{
    OptionParser parser;
    parser.addRequiredRemainingArgument("file");
    auto parseResult = parser.parse(invocation.arguments);
    if (!parseResult.success)
    {
        invocation.streams.output.print(usage(invocation.keyword));
        return CommandResult::completed(1);
    }

    ETString path = parseResult.options["file"][0].trim();

    if (dir_.exists(path) && dir_.isDirectory(path))
    {
        invocation.streams.output.print("path points to a directory\n");
        return CommandResult::completed(1);
    }

    ETFile file = dir_.open(path.c_str(), FILE_MODE_APPEND, true);
    if (!file.isOpen())
    {
        invocation.streams.output.print("failed to touch file\n");
        return CommandResult::completed(1);
    }

    file.close();
    invocation.streams.output.print(path + " touched\n");
    return CommandResult::completed(0);
}

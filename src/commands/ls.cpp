#include "ls.h"
#include "OptionParser.h"
#include <sstream>

using namespace EmbeddedTerminal::cmd;

ETString ls::usage(const ETString &keyword) const
{
    return "List the conntent of directories\n\n" +
           keyword + " - List the content of the current directory\n" +
           keyword + " <path> - List the content of the specified directory\n" +
           keyword + " -l (<path>) - List the content of the optional specified directory. Each entry gets a new line. Additional the size of each entry will be displayed\n";
}

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::ls::invoke(CommandInvocation &invocation)
{
    OptionParser parser;
    parser.addOption("-l", "--long", "List entries on separate lines with file sizes");
    parser.addOptionalRemainingArgument("path");

    auto parseResult = parser.parse(invocation.arguments);
    if (!parseResult.success)
    {
        invocation.streams.output.print(usage(invocation.keyword));
        return CommandResult::completed(ErrorCode::Missused);
    }

    const bool longListing = parseResult.options.find("--long") != parseResult.options.end() && !parseResult.options["--long"].empty();

    ETString pathArg = parseResult.options.find("path") != parseResult.options.end() ? parseResult.options["path"].size() > 0 ? parseResult.options["path"][0] : "" : "";
    Path path = dir_.pwd();
    if (!pathArg.empty() && !dir_.isDirectory(pathArg.c_str()))
    {
        invocation.streams.output.print(pathArg + " is not a directory!\n");
        return CommandResult::completed(ErrorCode::InvalidArgument);
    }

    if (!pathArg.empty())
    {
        path = dir_.pwd(pathArg.c_str());
    }

    auto content = dir_.ls(path);

    invocation.streams.output.print("Listing directory: " + path + "\n");
    for (const auto &entry : content)
    {
        if (longListing)
        {
            auto f = dir_.open(path + "/" + entry);
            invocation.streams.output.print(toETString(f.size()) + " Bytes\t" + entry.getName() + "\n");
            f.close();
        }
        else
        {
            invocation.streams.output.print(entry.getName() + "\t");
        }
    }
    invocation.streams.output.print("\n");
    return CommandResult::completed(ErrorCode::None);
}

ETVector<ETString> ls::getSuggestions(const ETString &partial) const
{
    // Delegate to DirectoryCompleter
    return completer_.getSuggestions(partial);
}

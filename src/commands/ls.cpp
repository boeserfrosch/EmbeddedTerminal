#include "ls.h"
#include "OptionParser.h"
#include <sstream>

using namespace EmbeddedTerminal::cmd;
ETString ls::trigger(const ETString &keyword, const ETString &additional)
{
    OptionParser parser;
    parser.addOption("-l", "--long", "List entries on separate lines with file sizes");
    parser.addOptionalRemainingArgument("path");

    auto parseResult = parser.parse(additional);
    if (!parseResult.success)
    {
        return "ls error: " + parseResult.errorMessage + "\n";
    }

    const bool longListing = parseResult.options.find("--long") != parseResult.options.end() && !parseResult.options["--long"].empty();

    ETString pathArg = parseResult.options.find("path") != parseResult.options.end() && !parseResult.options["path"].empty() ? parseResult.options["path"][0] : "";
    Path path = dir_.pwd();
    if (!pathArg.empty() && !dir_.isDirectory(pathArg.c_str()))
    {
        return pathArg + " is not a directory!\n";
    }

    if (!pathArg.empty())
    {
        path = dir_.pwd(pathArg.c_str());
    }

    auto content = dir_.ls(path);

    ETString result = path + "\n";
    for (const auto &entry : content)
    {
        if (longListing)
        {
            auto f = dir_.open(path + "/" + entry);
            result += toETString(f.size()) + " Bytes\t" + entry.getName() + "\n";
        }
        else
        {
            result += entry.getName() + "\t";
        }
    }
    result += "\n";
    return result;
}

ETString ls::usage(const ETString &keyword)
{
    return "List the conntent of directories\n\n" +
           keyword + " - List the content of the current directory\n" +
           keyword + " <path> - List the content of the specified directory\n" +
           keyword + " -l (<path>) - List the content of the optional specified directory. Each entry gets a new line. Additional the size of each entry will be displayed\n";
}

ETVector<ETString> ls::getSuggestions(const ETString &partial)
{
    // Delegate to DirectoryCompleter
    return completer_.getSuggestions(partial);
}

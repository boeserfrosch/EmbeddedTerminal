#include "rmdir.h"

using namespace EmbeddedTerminal::cmd;
ETString rmdir::trigger(const ETString &keyword, const ETString &additional)
{
    OptionParser parser;
    parser.addRequiredRemainingArgument("folder");
    auto parseResult = parser.parse(additional);
    if (!parseResult.success)
    {
        return "Error: " + parseResult.errorMessage + "\nUsage: " + usage(keyword);
    }

    ETString folderName = parseResult.options["folder"][0];

    if (!dir_.exists(folderName))
    {
        return folderName + " did not exists\n";
    }
    if (!dir_.isDirectory(folderName))
    {
        return folderName + " is not a directory\n";
    }
    if (!dir_.isEmpty(folderName.c_str()))
    {
        return folderName + " is not empty\n";
    }
    if (!dir_.rmdir(folderName))
    {
        return "Could not remove " + folderName + "\n";
    }
    return folderName + " removed\n";
}

ETString rmdir::usage(const ETString &keyword)
{
    return keyword + " <folder> - Remove the specified folder\n";
}

ETVector<ETString> rmdir::getSuggestions(const ETString &partial)
{
    return completer_.getSuggestions(partial);
}

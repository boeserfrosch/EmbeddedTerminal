#include "commands/cd.h"
#include "cd.h"

using namespace EmbeddedTerminal::cmd;

ETString _errorMessage[] = {
    "None",
    "Path does not exist",
    "Path is not a directory",
    "Failed to change directory"};

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::cd::execute(CommandInvocation &invocation)
{
    OptionParser parser;
    parser.addRequiredRemainingArgument("path");
    auto parseResult = parser.parse(invocation.arguments);
    if (!parseResult.success)
    {
        invocation.stderrChannel.print(parseResult.errorMessage + "\n" + usage(invocation.keyword));
        return CommandResult::completed(1); // Error code for missing argument
    }

    ETString path = parseResult.options["path"][0];
    CDerror result = changeDirectory_(path);
    if (result != CDerror::None)
    {
        invocation.stderrChannel.print(_errorMessage[result] + "\n");
        return CommandResult::completed(1); // Error code for invalid path
    }

    invocation.stdoutChannel.print("> " + dir_.pwd() + "\n");
    return CommandResult::completed(0); // Success
}

ETString cd::trigger(const ETString &keyword, const ETString &relPath)
{
    OptionParser parser;
    parser.addRequiredRemainingArgument("path");
    auto parseResult = parser.parse(relPath);
    if (!parseResult.success)
    {
        return parseResult.errorMessage + "\n" + usage(keyword);
    }

    ETString path = parseResult.options["path"][0];
    CDerror result = changeDirectory_(path);

    if (result != CDerror::None)
    {
        return _errorMessage[result] + "\n";
    }
    return "> " + dir_.pwd() + "\n"; // Success, no output
}

CDerror cd::checkPath_(const ETString &relPath)
{
    if (!dir_.exists(relPath.c_str()))
    {
        return CDerror::PathDoesNotExist;
    }
    else if (!dir_.isDirectory(relPath.c_str()))
    {
        return CDerror::PathNotDirectory;
    }
    return CDerror::None; // Path is valid
}

CDerror cd::changeDirectory_(const ETString &relPath)
{
    CDerror pathCheck = checkPath_(relPath);
    if (pathCheck != CDerror::None)
    {
        return pathCheck;
    }

    if (!dir_.cd(relPath.c_str()))
    {
        return CDerror::FailedToChangeDirectory; // Failed to change directory
    }
    return CDerror::None; // Success
}

ETString cd::usage(const ETString &keyword)
{
    return keyword + " <path> - Change the current directory relative to path\n";
}

ETVector<ETString> cd::getSuggestions(const ETString &partial)
{
    // Delegate to DirectoryCompleter
    return completer_.getSuggestions(partial);
}
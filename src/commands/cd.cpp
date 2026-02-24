#include "commands/cd.h"
#include "cd.h"

using namespace EmbeddedTerminal::cmd;

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::cd::execute(CommandInvocation &invocation)
{
    ETString path = invocation.arguments.trim();
    ETString checkResult = _checkPath(path);
    if (!checkResult.empty())
    {
        invocation.stderrChannel.print(checkResult);
        return CommandResult::completed(1); // Error code for invalid path
    }

    if (_dir.cd(path.c_str()))
    {
        invocation.stdoutChannel.print("> " + _dir.pwd() + "\n");
        return CommandResult::completed(0); // Success
    }
    else
    {
        invocation.stderrChannel.print("Failed to change directory\n");
        return CommandResult::completed(2); // Error code for failed cd
    }
}

ETString cd::trigger(const ETString &keyword, const ETString &relPath)
{
    ETString path = relPath.trim();
    ETString checkResult = _checkPath(path);
    if (!checkResult.empty())
    {
        return checkResult;
    }

    if (_dir.cd(path.c_str()))
    {
        return "> " + _dir.pwd() + "\n"; // Success, no output
    }
    else
    {
        return "Failed to change directory\n";
    }
}

ETString cd::_checkPath(const ETString &relPath)
{
    ETString path = relPath.trim();
    if (path.empty())
    {
        return "Expected parameter\n";
    }
    if (!_dir.exists(path.c_str()))
    {
        return path + " did not exist \n";
    }
    if (!_dir.isDirectory(path.c_str()))
    {
        return path + " is not a directory \n";
    }
    return "";
}

ETString cd::usage(const ETString &keyword)
{
    return keyword + " [path] - Change the current directory relative to path\n";
}
ETVector<ETString> cd::getSuggestions(const ETString &partial)
{
    // Delegate to DirectoryCompleter
    return _completer.getSuggestions(partial);
}
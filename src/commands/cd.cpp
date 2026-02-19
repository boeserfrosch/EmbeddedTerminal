#include "commands/cd.h"

using namespace EmbeddedTerminal::cmd;
ETString cd::trigger(const ETString &keyword, const ETString &relPath)
{
    ETString path = relPath.trim();
    if (path.empty())
    {
        return "Expected parameter\n" + usage(keyword);
    }
    if (!_dir.exists(path.c_str()))
    {
        return path + " did not exist \n";
    }
    if (!_dir.isDirectory(path.c_str()))
    {
        return path + " is not a directory \n";
    }
    if (_dir.cd(path.c_str()))
    {
        return "> " + _dir.pwd() + "\n";
    }
    else
    {
        return "error \n";
    }
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
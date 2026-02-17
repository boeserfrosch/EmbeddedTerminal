#include "commands/cd.h"

using namespace EmbeddedTerminal::cmd;
ETString cd::trigger(ETString &keyword, ETString &relPath)
{
    relPath = relPath.trim();
    if (relPath.empty())
    {
        return "Expected parameter\n" + usage(keyword);
    }
    if (!_dir.exists(relPath.c_str()))
    {
        return relPath + " did not exist \n";
    }
    if (!_dir.isDirectory(relPath.c_str()))
    {
        return relPath + " is not a directory \n";
    }
    if (_dir.cd(relPath.c_str()))
    {
        return "> " + _dir.pwd() + "\n";
    }
    else
    {
        return "error \n";
    }
}

ETString cd::usage(ETString &keyword)
{
    return keyword + " [path] - Change the current directory relative to path\n";
}

#include "rmdir.h"

using namespace EmbeddedTerminal::cmd;
ETString rmdir::trigger(ETString &keyword, ETString &additional)
{
    additional = additional.trim();
    if (additional.empty())
        return "Can not remove folder with no name\n";
    if (!_dir.exists(additional))
    {
        return additional + " did not exists\n";
    }
    if (!_dir.isDirectory(additional))
    {
        return additional + " is not a directory\n";
    }
    if (!_dir.isEmpty(additional.c_str()))
    {
        return additional + " is not empty\n";
    }
    if (!_dir.rmdir(additional))
    {
        return "Could not remove " + additional + "\n";
    }
    return additional + " removed\n";
}

ETString rmdir::usage(ETString &keyword)
{
    return keyword + " [folder] - Remove the specfied folder\n";
}

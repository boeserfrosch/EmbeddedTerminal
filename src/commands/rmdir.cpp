#include "rmdir.h"

using namespace EmbeddedTerminal::cmd;
ETString rmdir::trigger(const ETString &keyword, const ETString &additional)
{
    ETString folderName = additional.trim();
    if (folderName.empty())
        return "Can not remove folder with no name\n";
    if (!_dir.exists(folderName))
    {
        return folderName + " did not exists\n";
    }
    if (!_dir.isDirectory(folderName))
    {
        return folderName + " is not a directory\n";
    }
    if (!_dir.isEmpty(folderName.c_str()))
    {
        return folderName + " is not empty\n";
    }
    if (!_dir.rmdir(folderName))
    {
        return "Could not remove " + folderName + "\n";
    }
    return folderName + " removed\n";
}

ETString rmdir::usage(const ETString &keyword)
{
    return keyword + " [folder] - Remove the specfied folder\n";
}

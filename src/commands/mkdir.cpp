#include "mkdir.h"

#include <sstream>

using namespace EmbeddedTerminal::cmd;
ETString mkdir::trigger(const ETString &keyword, const ETString &additional)
{
    ETString folderName = additional.trim();
    if (folderName.empty())
        return "Can not create folder with no name\n";

    if (_dir.exists(folderName))
    {
        return folderName + " already exists\n";
    }

    auto result = _dir.mkdir(folderName.c_str());
    if (!result)
    {
        ETString ss;
        ss += "Could not create " + _dir.pwd(folderName.c_str()) + "\n";
        return ss;
    }
    return folderName + " created\n";
}

ETString mkdir::usage(const ETString &keyword)
{
    return keyword + " [folder] - Create the specified folder in the current directory\n";
}

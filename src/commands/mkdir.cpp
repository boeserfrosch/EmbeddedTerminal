#include "mkdir.h"

#include <sstream>

using namespace EmbeddedTerminal::cmd;
ETString mkdir::trigger(ETString &keyword, ETString &additional)
{
    additional = additional.trim();
    if (additional.empty())
        return "Can not create folder with no name\n";

    if (_dir.exists(additional))
    {
        return additional + " already exists\n";
    }

    auto result = _dir.mkdir(additional.c_str());
    if (!result)
    {
        ETString ss;
        ss += "Could not create " + _dir.pwd(additional.c_str()) + "\n";
        return ss;
    }
    return additional + " created\n";
}

ETString mkdir::usage(ETString &keyword)
{
    return keyword + " [folder] - Create the specified folder in the current directory\n";
}

#include "rm.h"

using namespace EmbeddedTerminal::cmd;
ETString rm::trigger(ETString &keyword, ETString &additional)
{
    additional = additional.trim();
    if (additional.empty())
    {
        return "Can not remove unspecified file!\n";
    }
    if (!_dir.exists(additional.c_str()))
    {
        return additional + " did not exist!\n";
    }
    if (_dir.isDirectory(additional))
    {
        return additional + "is not a file\n";
    }
    auto result = _dir.remove(additional.trim().c_str());
    if (result)
    {
        return additional + " removed\n";
    }
    return "Error on deleting\n";
}

ETString rm::usage(ETString &keyword)
{
    return keyword + " [file] - Remove the specified file\n";
}

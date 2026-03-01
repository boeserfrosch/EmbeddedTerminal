#include "rm.h"

using namespace EmbeddedTerminal::cmd;
ETString rm::trigger(const ETString &keyword, const ETString &additional)
{
    ETString fileName = additional.trim();
    if (fileName.empty())
    {
        return "Can not remove unspecified file!\n";
    }
    if (!dir_.exists(fileName.c_str()))
    {
        return fileName + " did not exist!\n";
    }
    if (dir_.isDirectory(fileName))
    {
        return fileName + "is not a file\n";
    }
    auto result = dir_.remove(fileName.c_str());
    if (result)
    {
        return fileName + " removed\n";
    }
    return "Error on deleting\n";
}

ETString rm::usage(const ETString &keyword)
{
    return keyword + " [file] - Remove the specified file\n";
}

ETVector<ETString> rm::getSuggestions(const ETString &partial)
{
    return completer_.getSuggestions(partial);
}

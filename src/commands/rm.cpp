#include "rm.h"

using namespace EmbeddedTerminal::cmd;
ETString rm::trigger(const ETString &keyword, const ETString &additional)
{
    ETString fileName = additional.trim();
    if (fileName.empty())
    {
        return "Can not remove unspecified file!\n";
    }
    if (!_dir.exists(fileName.c_str()))
    {
        return fileName + " did not exist!\n";
    }
    if (_dir.isDirectory(fileName))
    {
        return fileName + "is not a file\n";
    }
    auto result = _dir.remove(fileName.c_str());
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
    return _completer.getSuggestions(partial);
}

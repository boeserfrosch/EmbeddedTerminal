#include "tail.h"
using namespace EmbeddedTerminal::cmd;
ETString tail::trigger(ETString &keyword, ETString &additional)
{
    bool truncated = false;
    additional = additional.trim();
    if (additional.empty())
    {
        return "Expected parameter\n" + usage(keyword);
    }
    if (!_dir.exists(additional.c_str()) || _dir.isDirectory(additional.c_str()))
    {
        return "file " + additional + " did not exist!\n";
    }
    auto file = _dir.open(additional.trim().c_str(), "r", false);
    if (file.size() > 512)
    {
        truncated = true;
        file.seek(file.size() - 512);
    }
    auto content = file.readAll();
    file.close();
    if (truncated)
    {
        return "... File truncated ... \n" + content + "\n";
    }
    return ETString(content.c_str()) + "\n";
}

ETString tail::usage(ETString &keyword)
{
    return keyword + " [file] - Returns the last lines of the specified file. At max 512 bytes.\n";
}

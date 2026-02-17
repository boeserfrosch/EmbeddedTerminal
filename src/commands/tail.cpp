#include "tail.h"
using namespace EmbeddedTerminal::cmd;
ETString tail::trigger(const ETString &keyword, const ETString &additional)
{
    bool truncated = false;
    ETString fileName = additional.trim();
    if (fileName.empty())
    {
        return "Expected parameter\n" + usage(keyword);
    }
    if (!_dir.exists(fileName.c_str()) || _dir.isDirectory(fileName.c_str()))
    {
        return "file " + fileName + " did not exist!\n";
    }
    auto file = _dir.open(fileName.c_str(), "r", false);
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

ETString tail::usage(const ETString &keyword)
{
    return keyword + " [file] - Returns the last lines of the specified file. At max 512 bytes.\n";
}

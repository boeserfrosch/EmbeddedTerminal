#include "commands/cat.h"
ETString EmbeddedTerminal::cmd::cat::trigger(ETString &keyword, ETString &additional)
{
    additional = additional.trim();
    if (additional.empty())
    {
        return "path or name to file expected\n";
    }
    if (!_dir.exists(additional.c_str()) || _dir.isDirectory(additional.c_str()))
    {
        return "file " + additional + " did not exist!\n";
    }
    auto file = _dir.open(additional.c_str(), "r", false);
    if (file.size() < 512)
    {
        auto content = file.readAll();
        file.close();
        return ETString(content.c_str()) + "\n";
    }
    unsigned char buffer[512];
    file.read(buffer, 512);
    file.close();
    return ETString((char *)buffer) + "\n" + "... File truncated ...";
}

ETString EmbeddedTerminal::cmd::cat::usage(ETString &keyword)
{
    return keyword + " [file] - Returns the content of the defined file (at max the first 512 bytes)\n";
}

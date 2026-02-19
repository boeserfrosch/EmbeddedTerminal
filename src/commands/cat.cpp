#include "commands/cat.h"
ETString EmbeddedTerminal::cmd::cat::trigger(const ETString &keyword, const ETString &additional)
{
    ETString path = additional.trim();
    if (path.empty())
    {
        return "path or name to file expected\n";
    }
    if (!_dir.exists(path.c_str()) || _dir.isDirectory(path.c_str()))
    {
        return "file " + path + " did not exist!\n";
    }
    auto file = _dir.open(path.c_str(), "r", false);
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

ETString EmbeddedTerminal::cmd::cat::usage(const ETString &keyword)
{
    return keyword + " [file] - Returns the content of the defined file (at max the first 512 bytes)\n";
}

ETVector<ETString> EmbeddedTerminal::cmd::cat::getSuggestions(const ETString &partial)
{
    // Delegate to FilePathCompleter
    return _completer.getSuggestions(partial);
}

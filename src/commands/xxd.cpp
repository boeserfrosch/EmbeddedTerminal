#include "commands/xxd.h"
ETString EmbeddedTerminal::cmd::xxd::trigger(const ETString &keyword, const ETString &additional)
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
        return generateHexDump(content.c_str(), content.length()) + "\n";
    }

    char buffer[512];
    file.read(buffer, 512);
    file.close();
    ETString result = generateHexDump(buffer, 512);
    return result + "\n ... output truncated, file is larger than 512 bytes\n";
}

ETString EmbeddedTerminal::cmd::xxd::usage(const ETString &keyword)
{
    return keyword + " [file] - Returns the content of the defined file as hex dump\n";
}

ETVector<ETString> EmbeddedTerminal::cmd::xxd::getSuggestions(const ETString &partial)
{
    // Delegate to FilePathCompleter
    return _completer.getSuggestions(partial);
}

ETString EmbeddedTerminal::cmd::xxd::generateHexDump(const char *content, size_t length, size_t startOffset, size_t bytesPerLine)
{
    ETString result = "";
    for (size_t i = 0; i < length; i += bytesPerLine)
    {
        char offsetStr[9];
        sprintf(offsetStr, "%08X", static_cast<unsigned int>(startOffset + i));
        result += offsetStr;
        result += ": ";
        for (size_t j = 0; j < bytesPerLine && (i + j) < length; ++j)
        {
            char hex[3];
            sprintf(hex, "%02X", content[i + j]);
            result += hex;
            if ((j + 1) % 16 == 0)
            {
                result += "\n";
            }
            else
            {
                result += " ";
            }
        }
        result += "\n";
    }
    return result;
}
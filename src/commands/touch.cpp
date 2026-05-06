#include "commands/touch.h"

using namespace EmbeddedTerminal::cmd;

ETString touch::trigger(const ETString &keyword, const ETString &additional)
{
    OptionParser parser;
    parser.addRequiredRemainingArgument("file");
    auto parseResult = parser.parse(additional);
    if (!parseResult.success)
    {
        return "touch error: " + parseResult.errorMessage + "\n" + usage(keyword);
    }

    ETString path = parseResult.options["file"][0].trim();

    if (dir_.exists(path) && dir_.isDirectory(path))
    {
        return "path points to a directory\n";
    }

    ETFile file = dir_.open(path.c_str(), FILE_MODE_APPEND, true);
    if (!file.isOpen())
    {
        return "failed to touch file\n";
    }

    file.close();
    return path + " touched\n";
}

ETString touch::usage(const ETString &keyword)
{
    return keyword + " [file] - Create file if missing, otherwise leave existing file unchanged\n";
}

ETVector<ETString> touch::getSuggestions(const ETString &partial)
{
    return completer_.getSuggestions(partial);
}

#include "commands/wc.h"
#include <cctype>

using namespace EmbeddedTerminal::cmd;

wc::Counts wc::countText_(const ETString &text) const
{
    Counts counts;
    counts.bytes = text.length();

    bool inWord = false;
    for (size_t i = 0; i < text.length(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        if (c == '\n')
        {
            ++counts.lines;
        }

        if (std::isspace(c))
        {
            inWord = false;
        }
        else if (!inWord)
        {
            ++counts.words;
            inWord = true;
        }
    }

    return counts;
}

ETString wc::formatCounts_(const Counts &counts, const ETString &path) const
{
    ETString output = toETString(counts.lines) + " " + toETString(counts.words) + " " + toETString(counts.bytes);
    if (!path.empty())
    {
        output += " " + path;
    }
    return output + "\n";
}

EmbeddedTerminal::CommandResult wc::execute(EmbeddedTerminal::CommandInvocation &invocation)
{
    ETString path = invocation.arguments.trim();
    if (path.empty())
    {
        ETString input = invocation.stdinChannel.readAll();
        invocation.stdoutChannel.print(formatCounts_(countText_(input)));
        return CommandResult::completed(0);
    }

    if (!dir_.exists(path.c_str()))
    {
        invocation.stderrChannel.print("file did not exist\n");
        return CommandResult::completed(2);
    }
    if (dir_.isDirectory(path.c_str()))
    {
        invocation.stderrChannel.print("path points to a directory\n");
        return CommandResult::completed(3);
    }

    ETFile file = dir_.open(path.c_str(), FILE_MODE_READ, false);
    if (!file.isOpen())
    {
        invocation.stderrChannel.print("failed to open file\n");
        return CommandResult::completed(4);
    }

    ETString content = file.readAll();
    file.close();
    invocation.stdoutChannel.print(formatCounts_(countText_(content), path));
    return CommandResult::completed(0);
}

ETString wc::trigger(const ETString &keyword, const ETString &additional)
{
    OptionParser parser;
    parser.addOptionalRemainingArgument("file");
    auto parseResult = parser.parse(additional);
    if (!parseResult.success)
    {
        return "wc error: " + parseResult.errorMessage + "\n" + usage(keyword);
    }

    ETString path = parseResult.options["file"][0].trim();
    if (!dir_.exists(path.c_str()) || dir_.isDirectory(path.c_str()))
    {
        return "file " + path + " did not exist!\n";
    }

    ETFile file = dir_.open(path.c_str(), FILE_MODE_READ, false);
    if (!file.isOpen())
    {
        return "failed to open file\n";
    }

    ETString content = file.readAll();
    file.close();
    return formatCounts_(countText_(content), path);
}

ETString wc::usage(const ETString &keyword)
{
    return keyword + " [file] - Print line, word and byte counts (reads stdin when file is omitted)\n";
}

ETVector<ETString> wc::getSuggestions(const ETString &partial)
{
    return completer_.getSuggestions(partial);
}

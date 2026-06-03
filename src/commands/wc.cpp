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

EmbeddedTerminal::CommandResult wc::invoke(EmbeddedTerminal::CommandInvocation &invocation)
{
    OptionParser parser;
    parser.addOptionalRemainingArgument("file");

    auto parseResult = parser.parse(invocation.arguments);
    ETString path;
    auto fileOption = parseResult.options.find("file");
    if (fileOption != parseResult.options.end() && !fileOption->second.empty())
    {
        invocation.streams.output.print("File\tLines\tWords\tBytes\n");
        bool failed = false;
        Counts totalCounts;
        for (const auto &filePath : fileOption->second)
        {
            Counts counts;
            CommandResult result = countWordsInFile_(filePath, invocation, counts);
            if (result.state != CommandExecutionState::Completed || result.exitCode != 0)
            {
                // If we fail to count words in one file, we print an error but continue with the next files instead of aborting the entire command, this is similar to how the real wc behaves
                invocation.streams.error.print("Failed to count words in " + filePath + "\n");
                failed = true;
            }
            totalCounts.lines += counts.lines;
            totalCounts.words += counts.words;
            totalCounts.bytes += counts.bytes;
        }
        if (fileOption->second.size() > 1)
        {
            invocation.streams.output.print("-----------------------------------\n");
            invocation.streams.output.print("total:\t" + toETString(totalCounts.lines) + "\t" + toETString(totalCounts.words) + "\t" + toETString(totalCounts.bytes) + "\n");
        }
        if (failed)
        {
            return CommandResult::completed(1);
        }
        return CommandResult::completed(0);
    }

    return countWordsInStdin_(invocation);
}

EmbeddedTerminal::CommandResult wc::countWordsInStdin_(CommandInvocation &invocation)
{
    ETString input = invocation.streams.input.readAll();
    Counts counts = countText_(input);
    invocation.streams.output.print("Lines\tWords\tBytes\n");
    invocation.streams.output.print(toETString(counts.lines) + "\t" + toETString(counts.words) + "\t" + toETString(counts.bytes) + "\n");
    return CommandResult::completed(0);
}

EmbeddedTerminal::CommandResult wc::countWordsInFile_(const ETString &path, CommandInvocation &invocation, Counts &counts)
{
    if (!dir_.exists(path.c_str()))
    {
        invocation.streams.error.print("file did not exist\n");
        return CommandResult::completed(2);
    }
    if (dir_.isDirectory(path.c_str()))
    {
        invocation.streams.error.print("path points to a directory\n");
        return CommandResult::completed(3);
    }

    ETFile file = dir_.open(path.c_str(), FILE_MODE_READ, false);
    if (!file.isOpen())
    {
        invocation.streams.error.print("failed to open file\n");
        return CommandResult::completed(4);
    }

    // For now we read the entire file into memory since wc typically does this and it's simpler to implement, but we could enhance this later to support large files by reading and counting in chunks
    ETString content = file.readAll();
    file.close();
    counts = countText_(content);
    invocation.streams.output.print(path + ":\t" + toETString(counts.lines) + "\t" + toETString(counts.words) + "\t" + toETString(counts.bytes) + "\n");
    return CommandResult::completed(0);
}

ETString wc::usage(const ETString &keyword) const
{
    return keyword + " [[file]] - Print line, word and byte counts for each file (reads stdin when file is omitted)\n";
}

ETVector<ETString> wc::getSuggestions(const ETString &partial) const
{
    return completer_.getSuggestions(partial);
}

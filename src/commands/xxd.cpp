#include "commands/xxd.h"
#include "xxd.h"
ETString EmbeddedTerminal::cmd::xxd::usage(const ETString &keyword) const
{
    return keyword + " [file] - Returns the content of the defined file as hex dump\n";
}

ETVector<ETString> EmbeddedTerminal::cmd::xxd::getSuggestions(const ETString &partial) const
{
    // Delegate to FilePathCompleter
    FilePathCompleter completer(dir_);
    ETVector<ETString> suggestions = completer.getSuggestions(partial);
    for (auto &suggestion : suggestions)
    {
        if (!suggestion.empty() && suggestion[0] != '/')
        {
            suggestion = "/" + suggestion;
        }
    }
    return suggestions;
}

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::xxd::invoke(CommandInvocation &invocation)
{
    OptionParser parser;
    parser.addRequiredRemainingArgument("file");
    auto parseResult = parser.parse(invocation.arguments);
    if (!parseResult.success)
    {
        return error_(ErrorCode::INVALID_OPTIONS, invocation);
    }

    ETString filePath = parseResult.options["file"][0];

    if (filePath.empty())
    {
        return error_(ErrorCode::INVALID_PATH, invocation);
    }
    if (!dir_.exists(filePath.c_str()))
    {
        return error_(ErrorCode::FILE_NOT_FOUND, invocation);
    }
    if (dir_.isDirectory(filePath.c_str()))
    {
        return error_(ErrorCode::IS_DIRECTORY, invocation);
    }

    invocation.context.variables[SESSION_KEY_PATH] = filePath.trim();
    invocation.context.variables[SESSION_KEY_POS] = "0";

    return streamHexDump_(invocation);
}

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::xxd::resume(CommandInvocation &invocation)
{
    auto keyStrokeResult = handleKeyStrokes_(invocation);

    if (keyStrokeResult.exitCommand)
    {
        reset(invocation);
        return CommandResult::completed(ErrorCode::NONE);
    }
    if (keyStrokeResult.hasChanges)
    {
        return streamHexDump_(invocation);
    }
    return CommandResult::running(ErrorCode::NONE);
}

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::xxd::streamHexDump_(CommandInvocation &invocation)
{
    ETString filePath = invocation.context.variables[SESSION_KEY_PATH];
    size_t position = std::stoull(invocation.context.variables[SESSION_KEY_POS].c_str());

    auto file = dir_.open(filePath.c_str(), "r", false);
    auto code = checkFile_(file);
    if (code != ErrorCode::NONE)
    {
        file.close();
        return error_(code, invocation);
    }

    char chunkBuffer[CHUNK_SIZE];

    file.seek(position);
    size_t readBytes = file.read(chunkBuffer, CHUNK_SIZE);
    file.close();
    if (readBytes == 0)
    {
        return error_(ErrorCode::FAILED_TO_READ, invocation);
    }

    char offsetStr[9];
    char hex[3];
    for (size_t i = 0; i < readBytes; i += 16) // @todo 16 is the default bytesPerLine, we can enhance this later to make it configurable or at least a central constant
    {
        sprintf(offsetStr, "%08X", static_cast<unsigned int>(position + i));
        invocation.streams.output.print(offsetStr);
        invocation.streams.output.print(": ");
        for (size_t j = 0; j < 16 && (i + j) < readBytes; ++j) // @todo 16 is the default bytesPerLine, we can enhance this later to make it configurable or at least a central constant
        {
            sprintf(hex, "%02X", chunkBuffer[i + j]);
            invocation.streams.output.print(hex);
            if ((j + 1) % 16 == 0)
            {
                invocation.streams.output.print("\n");
            }
            else
            {
                invocation.streams.output.print(" ");
            }
        }
        invocation.streams.output.print("\n");
    }

    return CommandResult::running(ErrorCode::NONE);
}

EmbeddedTerminal::cmd::xxd::HandleKeyStrokesResult EmbeddedTerminal::cmd::xxd::handleKeyStrokes_(CommandInvocation &invocation)
{
    // We want to walk through the input and check for key strokes to navigate through the file (e.g., for pagination).
    // 'n' for next m bytes (bytesPerLine), 'p' for previous m bytes, 'q' to quit.
    // 'g' to go to the beginning, 'G' to go to the end.
    // 'o' for go to offset (followed by offset in hex, e.g., 'o1A3F')

    auto commitOffset = [&](HandleKeyStrokesResult &result)
    {
        if (pendingOffset_.empty())
        {
            collectingOffset_ = false;
            return;
        }

        size_t offset = std::stoull(pendingOffset_.c_str(), nullptr, 16);
        invocation.context.variables[SESSION_KEY_POS] = toETString(offset);
        pendingOffset_ = "";
        collectingOffset_ = false;
        result.hasChanges = true;
    };

    auto isHexDigit = [](char c)
    {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    };

    if (invocation.streams.input.available())
    {
        ETString input = invocation.streams.input.readAll();
        HandleKeyStrokesResult result;

        for (size_t index = 0; index < input.length();)
        {
            char c = input[index];

            if (c == '\r' || c == '\n' || c == ' ' || c == '\t')
            {
                if (collectingOffset_ && !pendingOffset_.empty())
                {
                    commitOffset(result);
                }
                ++index;
                continue;
            }

            if (collectingOffset_)
            {
                if (isHexDigit(c))
                {
                    pendingOffset_ += c;
                    ++index;
                    continue;
                }

                if (!pendingOffset_.empty())
                {
                    commitOffset(result);
                    continue;
                }

                collectingOffset_ = false;
            }

            if (c == 'o')
            {
                collectingOffset_ = true;
                pendingOffset_ = "";
                ++index;
                continue;
            }

            if (c == 'n')
            {
                // Next chunk
                auto file = dir_.open(invocation.context.variables[SESSION_KEY_PATH].c_str(), "r", false);
                auto fileSize = file.size();
                file.close();

                size_t currentPos = std::stoull(invocation.context.variables[SESSION_KEY_POS].c_str());
                if (currentPos + 16 > (fileSize - CHUNK_SIZE)) // @todo: 16 is the default bytesPerLine, we can enhance this later to make it configurable or at least a central constant
                {
                    invocation.context.variables[SESSION_KEY_POS] = toETString(fileSize - CHUNK_SIZE);
                }
                else
                {
                    invocation.context.variables[SESSION_KEY_POS] = toETString(currentPos + 16); // @todo 16 is the default bytesPerLine, we can enhance this later to make it configurable or at least a central constant
                }
                result.hasChanges = true;
                ++index;
                continue;
            }

            if (c == 'p')
            {
                // Previous chunk
                size_t currentPos = std::stoull(invocation.context.variables[SESSION_KEY_POS].c_str());
                if (currentPos >= 16) // @todo 16 is the default bytesPerLine, we can enhance this later to make it configurable or at least a central constant
                {
                    invocation.context.variables[SESSION_KEY_POS] = toETString(currentPos - 16); // @todo 16 is the default bytesPerLine, we can enhance this later to make it configurable or at least a central constant
                }
                else
                {
                    invocation.context.variables[SESSION_KEY_POS] = "0";
                }
                result.hasChanges = true;
                ++index;
                continue;
            }

            if (c == 'q')
            {
                // Quit
                result.exitCommand = true;
                break;
            }

            if (c == 'g')
            {
                // Go to beginning
                invocation.context.variables[SESSION_KEY_POS] = "0";
                result.hasChanges = true;
                ++index;
                continue;
            }

            if (c == 'G')
            {
                // Go to end
                auto file = dir_.open(invocation.context.variables[SESSION_KEY_PATH].c_str(), "r", false);
                auto fileSize = file.size();
                file.close();

                if (fileSize > CHUNK_SIZE)
                {
                    invocation.context.variables[SESSION_KEY_POS] = toETString(fileSize - CHUNK_SIZE);
                }
                else
                {
                    invocation.context.variables[SESSION_KEY_POS] = "0";
                }
                result.hasChanges = true;
                ++index;
                continue;
            }

            ++index;
        }

        return result;
    }

    HandleKeyStrokesResult result;
    result.hasChanges = false;
    result.exitCommand = false;
    return result;
}

EmbeddedTerminal::cmd::xxd::ErrorCode EmbeddedTerminal::cmd::xxd::checkFile_(ETFile &file)
{
    if (!file.isOpen())
    {
        return ErrorCode::FAILED_TO_OPEN_FILE;
    }
    if (!file.seek(0))
    {
        file.close();
        return ErrorCode::FAILED_TO_SEEK;
    }
    return ErrorCode::NONE;
}

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::xxd::error_(ErrorCode errorCode, CommandInvocation &invocation)
{
    reset(invocation);
    switch (errorCode)
    {
    case ErrorCode::INVALID_OPTIONS:
        invocation.streams.error.print("invalid options\n");
        invocation.streams.output.print(usage(invocation.keyword));
        break;
    case ErrorCode::INVALID_PATH:
        invocation.streams.error.print("path or name to file expected\n");
        invocation.streams.output.print(usage(invocation.keyword));
        break;
    case ErrorCode::FILE_NOT_FOUND:
        invocation.streams.error.print("file not found\n");
        break;
    case ErrorCode::IS_DIRECTORY:
        invocation.streams.error.print("path points to a directory\n");
        break;
    case ErrorCode::FAILED_TO_OPEN_FILE:
        invocation.streams.error.print("failed to open file\n");
        break;
    case ErrorCode::FAILED_TO_SEEK:
        invocation.streams.error.print("failed to seek file\n");
        break;
    case ErrorCode::FAILED_TO_READ:
        invocation.streams.error.print("failed to read file\n");
        break;
    case ErrorCode::FAILED_TO_ALLOCATE_BUFFER:
        invocation.streams.error.print("failed to allocate buffer\n");
        break;
    default:
        break;
    }

    return CommandResult::completed(static_cast<int>(errorCode));
}

void EmbeddedTerminal::cmd::xxd::reset(CommandInvocation &invocation)
{
    invocation.context.variables.erase(SESSION_KEY_PATH);
    invocation.context.variables.erase(SESSION_KEY_POS);
    pendingOffset_ = "";
    collectingOffset_ = false;
}

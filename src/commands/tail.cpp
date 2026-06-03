#include "tail.h"
using namespace EmbeddedTerminal::cmd;

ETString tail::usage(const ETString &keyword) const
{
    return keyword + " [file] - Returns the last n lines of the specified file.\n" +
           "By default, n is 10, but you can specify a different number of lines by adding -n [number] before the file name.\n" +
           "Example: tail -n 20 /logs/system.log\n" +
           "Or tail --lines 20 /logs/system.log\n";
}

ETVector<ETString> tail::getSuggestions(const ETString &partial) const
{
    return completer_.getSuggestions(partial);
}

EmbeddedTerminal::CommandResult tail::invoke(CommandInvocation &invocation)
{
    auto parseResult = parseOptions_(invocation);
    if (parseResult.state != CommandExecutionState::Running)
    {
        return parseResult;
    }

    auto findPosResult = findStartPosition_(invocation);
    if (findPosResult.state != CommandExecutionState::Running)
    {
        return findPosResult;
    }

    return streamFile_(invocation);
}

EmbeddedTerminal::CommandResult tail::resume(CommandInvocation &invocation)
{
    if (invocation.context.variables.find(SESSION_KEY_PATH) == invocation.context.variables.end() ||
        invocation.context.variables.find(SESSION_KEY_POS) == invocation.context.variables.end())
    {
        return error_(ErrorCode::INVALID_RESUME, invocation);
    }
    return streamFile_(invocation);
}

EmbeddedTerminal::CommandResult tail::parseOptions_(CommandInvocation &invocation)
{
    OptionParser parser;
    parser.addOption("-n", "--lines", "Number of lines to display from the end of the file", true);
    // We expect that just one file name will be provided, we could enhance this later to support multiple files
    parser.addRequiredRemainingArgument("file");
    auto parseResult = parser.parse(invocation.arguments);

    if (!parseResult.success)
    {
        return error_(ErrorCode::INVALID_OPTIONS, invocation);
    }

    linesToFind_ = 10; // Default to last 10 lines, this should be configurable or at least a central constant
    if (parseResult.options.count("--lines") && !parseResult.options["--lines"].empty())
    {
        linesToFind_ = std::stoi(parseResult.options["--lines"][0]);
    }

    if (linesToFind_ <= 0)
    {
        return error_(ErrorCode::INVALID_NUMBER_OF_LINES, invocation);
    }

    // For now we expect the file name as the first remaining argument after options, we could enhance this later to support more flexible argument ordering or multiple files
    ETString fileName = parseResult.options["file"][0];
    if (fileName.empty())
    {
        return error_(ErrorCode::INVALID_OPTIONS, invocation);
    }
    if (!dir_.exists(fileName.c_str()) || dir_.isDirectory(fileName.c_str()))
    {
        return error_(ErrorCode::FILE_NOT_FOUND, invocation);
    }
    auto file = dir_.open(fileName.c_str(), "r", false);
    if (!file.isOpen())
    {
        return error_(ErrorCode::FAILED_TO_OPEN_FILE, invocation);
    }

    invocation.context.variables[SESSION_KEY_PATH] = fileName;
    invocation.context.variables[SESSION_KEY_POS] = "0";

    return CommandResult::running(0);
}

EmbeddedTerminal::CommandResult tail::findStartPosition_(CommandInvocation &invocation)
{
    ETString fileName = invocation.context.variables[SESSION_KEY_PATH];
    size_t seekOffsetPos = 0; // How far from the end of the file we have seeked while looking for the starting position to stream from

    auto file = dir_.open(fileName.c_str(), "r", false);
    if (!file.isOpen())
    {
        return error_(ErrorCode::FAILED_TO_OPEN_FILE, invocation);
    }

    if (!file.seek(0))
    {
        file.close();
        return error_(ErrorCode::FAILED_TO_SEEK, invocation);
    }

    size_t fileSize = file.size();
    if (fileSize == 0)
    {
        file.close();
        return success_(invocation); // Empty file, nothing to find but it's not an error
    }

    const size_t bufferSize = 512; // @todo: we could make this buffer size configurable or at least make it a central constant
    unsigned char buffer[bufferSize];

    size_t linesFound = 0;
    while (seekOffsetPos < fileSize && linesFound < linesToFind_)
    {
        size_t pos = fileSize - seekOffsetPos;
        size_t chunkSize = (pos <= bufferSize) ? pos : bufferSize;
        pos -= chunkSize;
        file.seek(pos);

        size_t bytesRead = file.read(buffer, chunkSize);
        if (bytesRead == 0)
        {
            file.close();
            return error_(ErrorCode::FAILED_TO_READ, invocation);
        }

        size_t i = bytesRead;
        do
        {
            i--;
            if (buffer[i] == '\n')
            {
                linesFound++;
                if (linesFound >= linesToFind_)
                {
                    seekOffsetPos = fileSize - (pos + i + 1); // Position after the newline
                    invocation.context.variables[SESSION_KEY_POS] = toETString(seekOffsetPos);
                    file.close();
                    return CommandResult::running(ErrorCode::NONE);
                }
            }
        } while (i > 0);
    }

    // If we reached the beginning of the file, we should stream from the start
    invocation.context.variables[SESSION_KEY_POS] = toETString(fileSize);
    file.close();

    return CommandResult::running(ErrorCode::NONE);
}

EmbeddedTerminal::CommandResult tail::streamFile_(CommandInvocation &invocation)
{
    ETString fileName = invocation.context.variables[SESSION_KEY_PATH];
    size_t fileEndPosOffset = ETString::toull(invocation.context.variables[SESSION_KEY_POS].c_str());

    auto file = dir_.open(fileName.c_str(), "r", false);
    if (!file.isOpen())
    {
        return error_(ErrorCode::FAILED_TO_OPEN_FILE, invocation);
    }

    auto fileSize = file.size();
    if (fileSize == 0)
    {
        file.close();
        return success_(invocation); // Empty file, nothing to stream but it's not an error
    }

    if (fileEndPosOffset > fileSize)
    {
        file.close();
        return error_(ErrorCode::FAILED_TO_SEEK, invocation);
    }

    if (!file.seek(fileSize - fileEndPosOffset))
    {
        file.close();
        return error_(ErrorCode::FAILED_TO_SEEK, invocation);
    }

    // We emit in chunks for non blocking behavior and to support large files and slow storage or long lines that exceed the buffer size
    const size_t bufferSize = 512;
    unsigned char buffer[bufferSize];
    size_t bytesRead = file.read(buffer, bufferSize);
    if (bytesRead == 0)
    {
        file.close();
        return error_(ErrorCode::FAILED_TO_READ, invocation);
    }

    ETString chunk(std::string(reinterpret_cast<const char *>(buffer), bytesRead));
    invocation.streams.output.print(chunk);

    size_t newPos = fileEndPosOffset - bytesRead;
    invocation.context.variables[SESSION_KEY_POS] = toETString(newPos);

    file.close();

    if (newPos == 0)
    {
        return success_(invocation);
    }
    else
    {
        return CommandResult::running(ErrorCode::NONE);
    }
}

void EmbeddedTerminal::cmd::tail::reset(CommandInvocation &invocation)
{
    invocation.context.variables.erase(SESSION_KEY_PATH);
    invocation.context.variables.erase(SESSION_KEY_POS);
}

EmbeddedTerminal::CommandResult tail::error_(size_t errorCode, CommandInvocation &invocation)
{
    reset(invocation); // Clear any state related to the command since we're in an error state and want to avoid leaving stale state that could interfere with the next execution

    switch (errorCode)
    {
    case ErrorCode::INVALID_OPTIONS:
        invocation.streams.error.print("Invalid options. Usage:\n" + usage(invocation.keyword));
        break;
    case ErrorCode::INVALID_NUMBER_OF_LINES:
        invocation.streams.error.print("Invalid number of lines specified. It must be a positive integer.");
        break;
    case ErrorCode::FILE_NOT_FOUND:
        invocation.streams.error.print("File did not exist.");
        break;
    case ErrorCode::IS_DIRECTORY:
        invocation.streams.error.print("Specified path is a directory, not a file.");
        break;
    case ErrorCode::FAILED_TO_OPEN_FILE:
        invocation.streams.error.print("Failed to open file.");
        break;
    case ErrorCode::FAILED_TO_SEEK:
        invocation.streams.error.print("Failed to seek in file.");
        break;
    case ErrorCode::FAILED_TO_READ:
        invocation.streams.error.print("Failed to read from file.");
        break;
    default:
        invocation.streams.error.print("An unknown error occurred.");
        break;
    }

    return CommandResult::completed(errorCode);
}

EmbeddedTerminal::CommandResult tail::success_(CommandInvocation &invocation)
{
    reset(invocation); // Clear any state related to the command since we're done and want to avoid leaving stale state that could interfere with the next execution
    return CommandResult::completed(ErrorCode::NONE);
}
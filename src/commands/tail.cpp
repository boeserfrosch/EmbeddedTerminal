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
    if (!dir_.exists(fileName.c_str()) || dir_.isDirectory(fileName.c_str()))
    {
        return "file " + fileName + " did not exist!\n";
    }
    auto file = dir_.open(fileName.c_str(), "r", false);
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
    return keyword + " [file] - Returns the last n lines of the specified file.\n" +
           "By default, n is 10, but you can specify a different number of lines by adding -n [number] before the file name.\n" +
           "Example: tail -n 20 /logs/system.log\n" +
           "Or tail --lines 20 /logs/system.log\n";
}

ETVector<ETString> tail::getSuggestions(const ETString &partial)
{
    return completer_.getSuggestions(partial);
}

EmbeddedTerminal::CommandResult tail::execute(CommandInvocation &invocation)
{
    auto state = getState_(invocation);

    switch (state)
    {
    case TailState::Initial:
        return parseOptions_(invocation);
    case TailState::FindingStartPosition:
        return findStartPosition_(invocation);
    case TailState::Streaming:
        return streamFile_(invocation);
    }
    return CommandResult::completed(errorCodes::TAIL_CMD_ERROR_NONE);
}

tail::TailState tail::getState_(CommandInvocation &invocation)
{
    auto it = invocation.context.variables.find(SESSION_KEY_STATE);
    if (it == invocation.context.variables.end())
    {
        return TailState::Initial;
    }
    ETString stateStr = it->second;
    if (stateStr == "FindingStartPosition")
    {
        return TailState::FindingStartPosition;
    }
    else if (stateStr == "Streaming")
    {
        return TailState::Streaming;
    }
    // Default to Initial if state is unrecognized
    return TailState::Initial;
}

EmbeddedTerminal::CommandResult tail::parseOptions_(CommandInvocation &invocation)
{
    OptionParser parser;
    parser.addOption("-n", "--lines", "Number of lines to display from the end of the file", true);
    auto parseResult = parser.parse(invocation.arguments);

    if (!parseResult.success)
    {
        return error_(errorCodes::TAIL_CMD_ERROR_INVALID_OPTIONS, invocation);
    }

    size_t linesToFind = 10; // Default to last 10 lines
    if (parseResult.options.count("--lines") && !parseResult.options["--lines"].empty())
    {
        linesToFind = std::stoi(parseResult.options["--lines"][0]);
    }

    if (linesToFind <= 0)
    {
        return error_(errorCodes::TAIL_CMD_ERROR_INVALID_NUMBER_OF_LINES, invocation);
    }

    ETString fileName = parseResult.remainingArguments.trim();
    printf("Parsed options: linesToFind=%zu, fileName='%s'\n", linesToFind, fileName.c_str());
    if (fileName.empty())
    {
        return error_(errorCodes::TAIL_CMD_ERROR_INVALID_OPTIONS, invocation);
    }
    if (!dir_.exists(fileName.c_str()) || dir_.isDirectory(fileName.c_str()))
    {
        return error_(errorCodes::TAIL_CMD_ERROR_FILE_NOT_FOUND, invocation);
    }
    auto file = dir_.open(fileName.c_str(), "r", false);
    if (!file.isOpen())
    {
        return error_(errorCodes::TAIL_CMD_ERROR_FAILED_TO_OPEN_FILE, invocation);
    }

    invocation.context.variables[SESSION_KEY_PATH] = fileName;
    invocation.context.variables[SESSION_KEY_POS] = "0";
    invocation.context.variables[SESSION_KEY_STATE] = "FindingStartPosition";
    invocation.context.variables[SESSION_KEY_LINES_TO_FIND] = toETString(linesToFind);
    invocation.context.variables[SESSION_KEY_LINES_FOUND] = "0";

    return CommandResult::running(0);
}

EmbeddedTerminal::CommandResult tail::findStartPosition_(CommandInvocation &invocation)
{
    ETString fileName = invocation.context.variables[SESSION_KEY_PATH];
    size_t linesToFind = ETString::toull(invocation.context.variables[SESSION_KEY_LINES_TO_FIND].c_str());
    size_t seekOffsetPos = ETString::toull(invocation.context.variables[SESSION_KEY_POS].c_str());
    size_t linesFound = ETString::toull(invocation.context.variables[SESSION_KEY_LINES_FOUND].c_str());

    auto file = dir_.open(fileName.c_str(), "r", false);
    if (!file.isOpen())
    {
        return error_(errorCodes::TAIL_CMD_ERROR_FAILED_TO_OPEN_FILE, invocation);
    }

    if (!file.seek(0))
    {
        file.close();
        return error_(errorCodes::TAIL_CMD_ERROR_FAILED_TO_SEEK, invocation);
    }

    if (seekOffsetPos >= file.size())
    {
        invocation.context.variables[SESSION_KEY_POS] = toETString(file.size());
        invocation.context.variables[SESSION_KEY_STATE] = "Streaming";
        return CommandResult::running(errorCodes::TAIL_CMD_ERROR_NONE);
    }

    // Start from the end of the file and count newlines to find the starting position,
    // we should do this non blocking and in chunks to support large files and large number of lines or files on slow storage or long lines that exceed the buffer size
    size_t fileSize = file.size();
    size_t pos = fileSize - seekOffsetPos;

    const size_t bufferSize = 512;
    unsigned char buffer[bufferSize];

    size_t chunkSize = (pos <= bufferSize) ? pos : bufferSize;
    pos -= chunkSize;
    file.seek(pos);
    size_t bytesRead = file.read(buffer, chunkSize);
    if (bytesRead == 0)
    {
        file.close();
        return error_(errorCodes::TAIL_CMD_ERROR_FAILED_TO_READ, invocation);
    }

    for (ssize_t i = bytesRead - 1; i >= 0; i--)
    {
        if (buffer[i] == '\n')
        {
            linesFound++;
            if (linesFound >= linesToFind)
            {
                seekOffsetPos = fileSize - (pos + i + 1); // Position after the newline
                invocation.context.variables[SESSION_KEY_POS] = toETString(seekOffsetPos);
                invocation.context.variables[SESSION_KEY_STATE] = "Streaming";
                invocation.context.variables[SESSION_KEY_LINES_FOUND] = toETString(linesFound);

                file.close();
                return CommandResult::running(errorCodes::TAIL_CMD_ERROR_NONE);
            }
        }
    }

    file.close();

    invocation.context.variables[SESSION_KEY_POS] = toETString(fileSize - pos);
    invocation.context.variables[SESSION_KEY_LINES_FOUND] = toETString(linesFound);
    return CommandResult::running(errorCodes::TAIL_CMD_ERROR_NONE);
}

EmbeddedTerminal::CommandResult tail::streamFile_(CommandInvocation &invocation)
{
    ETString fileName = invocation.context.variables[SESSION_KEY_PATH];
    size_t fileEndPosOffset = ETString::toull(invocation.context.variables[SESSION_KEY_POS].c_str());

    auto file = dir_.open(fileName.c_str(), "r", false);
    if (!file.isOpen())
    {
        return error_(errorCodes::TAIL_CMD_ERROR_FAILED_TO_OPEN_FILE, invocation);
    }

    auto fileSize = file.size();
    if (fileEndPosOffset > fileSize)
    {
        file.close();
        return error_(errorCodes::TAIL_CMD_ERROR_FAILED_TO_SEEK, invocation);
    }

    if (!file.seek(fileSize - fileEndPosOffset))
    {
        file.close();
        return error_(errorCodes::TAIL_CMD_ERROR_FAILED_TO_SEEK, invocation);
    }

    // We emit in chunks for non blocking behavior and to support large files and slow storage or long lines that exceed the buffer size
    const size_t bufferSize = 512;
    unsigned char buffer[bufferSize];
    size_t bytesRead = file.read(buffer, bufferSize);
    if (bytesRead == 0)
    {
        file.close();
        return error_(errorCodes::TAIL_CMD_ERROR_FAILED_TO_READ, invocation);
    }

    ETString chunk(std::string(reinterpret_cast<const char *>(buffer), bytesRead));
    invocation.stdoutChannel.print(chunk);

    size_t newPos = fileEndPosOffset - bytesRead;
    invocation.context.variables[SESSION_KEY_POS] = toETString(newPos);

    file.close();

    if (newPos == 0)
    {
        return success_(invocation);
    }
    else
    {
        return CommandResult::running(errorCodes::TAIL_CMD_ERROR_NONE);
    }
}

EmbeddedTerminal::CommandResult tail::error_(size_t errorCode, CommandInvocation &invocation)
{
    invocation.context.variables.erase(SESSION_KEY_PATH);
    invocation.context.variables.erase(SESSION_KEY_POS);
    invocation.context.variables.erase(SESSION_KEY_STATE);
    invocation.context.variables.erase(SESSION_KEY_LINES_TO_FIND);
    invocation.context.variables.erase(SESSION_KEY_LINES_FOUND);

    switch (errorCode)
    {
    case errorCodes::TAIL_CMD_ERROR_INVALID_OPTIONS:
        invocation.stderrChannel.print("Invalid options. Usage:\n" + usage(invocation.keyword));
        break;
    case errorCodes::TAIL_CMD_ERROR_INVALID_NUMBER_OF_LINES:
        invocation.stderrChannel.print("Invalid number of lines specified. It must be a positive integer.");
        break;
    case errorCodes::TAIL_CMD_ERROR_FILE_NOT_FOUND:
        invocation.stderrChannel.print("File did not exist.");
        break;
    case errorCodes::TAIL_CMD_ERROR_IS_DIRECTORY:
        invocation.stderrChannel.print("Specified path is a directory, not a file.");
        break;
    case errorCodes::TAIL_CMD_ERROR_FAILED_TO_OPEN_FILE:
        invocation.stderrChannel.print("Failed to open file.");
        break;
    case errorCodes::TAIL_CMD_ERROR_FAILED_TO_SEEK:
        invocation.stderrChannel.print("Failed to seek in file.");
        break;
    case errorCodes::TAIL_CMD_ERROR_FAILED_TO_READ:
        invocation.stderrChannel.print("Failed to read from file.");
        break;
    default:
        invocation.stderrChannel.print("An unknown error occurred.");
        break;
    }

    return CommandResult::completed(errorCode);
}

EmbeddedTerminal::CommandResult tail::success_(CommandInvocation &invocation)
{
    invocation.context.variables.erase(SESSION_KEY_PATH);
    invocation.context.variables.erase(SESSION_KEY_POS);
    invocation.context.variables.erase(SESSION_KEY_STATE);
    invocation.context.variables.erase(SESSION_KEY_LINES_TO_FIND);
    invocation.context.variables.erase(SESSION_KEY_LINES_FOUND);
    return CommandResult::completed(errorCodes::TAIL_CMD_ERROR_NONE);
}
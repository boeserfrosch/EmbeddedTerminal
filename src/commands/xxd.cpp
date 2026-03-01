#include "commands/xxd.h"
ETString EmbeddedTerminal::cmd::xxd::trigger(const ETString &keyword, const ETString &additional)
{
    Path path = additional.trim();
    if (path.isEmpty())
    {
        return "path or name to file expected\n";
    }
    if (!dir_.exists(path))
    {
        return "file " + path + " did not exist!\n";
    }
    if (dir_.isDirectory(path))
    {
        return "file " + path + " did not exist!\n";
    }
    auto file = dir_.open(path, "r", false);
    if (file.size() < 512)
    {
        auto content = file.readAll();
        file.close();
        return generateHexDump_(content.c_str(), content.length()) + "\n";
    }

    char buffer[512];
    file.read(buffer, 512);
    file.close();
    ETString result = generateHexDump_(buffer, 512);
    return result + "\n ... output truncated, file is larger than 512 bytes\n";
}

ETString EmbeddedTerminal::cmd::xxd::usage(const ETString &keyword)
{
    return keyword + " [file] - Returns the content of the defined file as hex dump\n";
}

ETVector<ETString> EmbeddedTerminal::cmd::xxd::getSuggestions(const ETString &partial)
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

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::xxd::execute(CommandInvocation &invocation)
{
    auto state = handleCommandState_(invocation);
    errorCodes::XXDCmdErrorCode code = checkCommandState_(state, invocation);

    if (code != errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_NONE)
    {
        return error_(code, invocation);
    }

    return streamHexDump_(invocation, state);
}

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::xxd::streamHexDump_(CommandInvocation &invocation, const XXDState &state)
{
    char chunkBuffer[CHUNK_SIZE];

    auto executionState = handleKeyStrokes_(invocation);

    if (executionState.exitCommand)
    {
        return CommandResult::completed(errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_NONE);
    }

    size_t effectivePosition = state.position;
    if (executionState.hasChanges)
    {
        auto posIt = invocation.context.variables.find(SESSION_KEY_POS);
        if (posIt != invocation.context.variables.end())
        {
            effectivePosition = std::stoull(posIt->second.c_str());
        }
    }

    if (executionState.hasChanges || effectivePosition == 0)
    {
        auto file = dir_.open(state.path.c_str(), "r", false);
        auto code = checkFile_(file);
        if (code != errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_NONE)
        {
            file.close();
            return error_(code, invocation);
        }

        file.seek(effectivePosition);

        size_t readBytes = file.read(chunkBuffer, CHUNK_SIZE);
        if (readBytes == 0)
        {
            file.close();
            return error_(errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_FAILED_TO_READ, invocation);
        }
        file.close();

        invocation.stdoutChannel.print(generateHexDump_(chunkBuffer, readBytes, effectivePosition));
    }

    return CommandResult::completed(errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_NONE);
}

EmbeddedTerminal::cmd::xxd::HandleKeyStrokesResult EmbeddedTerminal::cmd::xxd::handleKeyStrokes_(CommandInvocation &invocation)
{
    // We want to walk through the input and check for key strokes to navigate through the file (e.g., for pagination).
    // 'n' for next m bytes (bytesPerLine), 'p' for previous m bytes, 'q' to quit.
    // 'g' to go to the beginning, 'G' to go to the end.
    // 'o' for go to offset (followed by offset in hex, e.g., 'o1A3F')

    if (invocation.stdinChannel.available())
    {
        ETString input = invocation.stdinChannel.readAll().trim();
        if (input == "n")
        {
            auto file = dir_.open(invocation.context.variables[SESSION_KEY_PATH].c_str(), "r", false);
            auto fileSize = file.size();
            file.close();

            size_t currentPos = std::stoull(invocation.context.variables[SESSION_KEY_POS].c_str());
            if (currentPos + 16 > (fileSize - CHUNK_SIZE))
            {
                invocation.context.variables[SESSION_KEY_POS] = toETString(fileSize - CHUNK_SIZE);
            }
            else
            {
                invocation.context.variables[SESSION_KEY_POS] = toETString(currentPos + 16);
            }
            HandleKeyStrokesResult result;
            result.hasChanges = true;
            result.exitCommand = false;
            return result;
        }
        else if (input == "p")
        {
            size_t currentPos = std::stoull(invocation.context.variables[SESSION_KEY_POS].c_str());
            if (currentPos >= 16)
            {
                invocation.context.variables[SESSION_KEY_POS] = toETString(currentPos - 16);
            }
            else
            {
                invocation.context.variables[SESSION_KEY_POS] = "0";
            }
            HandleKeyStrokesResult result;
            result.hasChanges = true;
            result.exitCommand = false;
            return result;
        }
        else if (input == "q")
        {
            // Quit
            HandleKeyStrokesResult result;
            result.hasChanges = false;
            result.exitCommand = true;
            return result;
        }
        else if (input == "g")
        {
            // Go to beginning
            invocation.context.variables[SESSION_KEY_POS] = "0";
            HandleKeyStrokesResult result;
            result.hasChanges = true;
            result.exitCommand = false;
            return result;
        }
        else if (input == "G")
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
            HandleKeyStrokesResult result;
            result.hasChanges = true;
            result.exitCommand = false;
            return result;
        }
        else if (input.startsWith("o"))
        {
            ETString offsetStr = input.substr(1).trim();
            size_t offset = std::stoull(offsetStr.c_str(), nullptr, 16);
            invocation.context.variables[SESSION_KEY_POS] = toETString(offset);
            HandleKeyStrokesResult result;
            result.hasChanges = true;
            result.exitCommand = false;
            return result;
        }
    }

    HandleKeyStrokesResult result;
    result.hasChanges = false;
    result.exitCommand = false;
    return result;
}

/* @brief Generates a hex dump string for the given content
 *
 *  @param content The binary content to be dumped
 *  @param length The length of the content in bytes
 *  @param startOffset The starting offset to display in the hex dump (default: 0)
 *  @param bytesPerLine The number of bytes to display per line (default: 16)
 *
 * @return A formatted hex dump string representing the content
 */
ETString EmbeddedTerminal::cmd::xxd::generateHexDump_(const char *content, size_t length, size_t startOffset, size_t bytesPerLine)
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

EmbeddedTerminal::cmd::xxd::XXDState EmbeddedTerminal::cmd::xxd::handleCommandState_(CommandInvocation &invocation)
{
    XXDState state;

    auto &vars = invocation.context.variables;
    auto pathIt = vars.find(SESSION_KEY_PATH);
    auto posIt = vars.find(SESSION_KEY_POS);

    if (pathIt != vars.end())
    {
        state.path = pathIt->second;
        if (posIt != vars.end())
        {
            state.position = std::stoull(posIt->second.c_str());
            state.initialState = false;
        }
    }
    else
    {
        state.path = invocation.arguments.trim();
        vars[SESSION_KEY_PATH] = state.path;
        vars[SESSION_KEY_POS] = "0";
    }
    return state;
}

EmbeddedTerminal::cmd::errorCodes::XXDCmdErrorCode EmbeddedTerminal::cmd::xxd::checkCommandState_(const XXDState &state, CommandInvocation &invocation)
{
    if (state.path.empty())
    {
        return errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_INVALID_PATH;
    }
    if (!dir_.exists(state.path.c_str()))
    {
        return errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_FILE_NOT_FOUND;
    }
    if (dir_.isDirectory(state.path.c_str()))
    {
        return errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_IS_DIRECTORY;
    }
    auto file = dir_.open(state.path.c_str(), "r", false);
    if (!file.isOpen())
    {
        return errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_FAILED_TO_OPEN_FILE;
    }
    if (!file.seek(state.position))
    {
        file.close();
        return errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_FAILED_TO_SEEK;
    }
    file.close();
    return errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_NONE;
}

EmbeddedTerminal::cmd::errorCodes::XXDCmdErrorCode EmbeddedTerminal::cmd::xxd::checkFile_(ETFile &file)
{
    if (!file.isOpen())
    {
        return errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_FAILED_TO_OPEN_FILE;
    }
    if (!file.seek(0))
    {
        file.close();
        return errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_FAILED_TO_SEEK;
    }
    return errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_NONE;
}

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::xxd::error_(errorCodes::XXDCmdErrorCode errorCode, CommandInvocation &invocation)
{
    switch (errorCode)
    {
    case errorCodes::XXD_CMD_ERROR_INVALID_OPTIONS:
        invocation.stderrChannel.print("invalid options\n");
        break;
    case errorCodes::XXD_CMD_ERROR_INVALID_PATH:
        invocation.stderrChannel.print("path or name to file expected\n");
        break;
    case errorCodes::XXD_CMD_ERROR_FILE_NOT_FOUND:
        invocation.stderrChannel.print("file not found\n");
        break;
    case errorCodes::XXD_CMD_ERROR_IS_DIRECTORY:
        invocation.stderrChannel.print("path points to a directory\n");
        break;
    case errorCodes::XXD_CMD_ERROR_FAILED_TO_OPEN_FILE:
        invocation.stderrChannel.print("failed to open file\n");
        break;
    case errorCodes::XXD_CMD_ERROR_FAILED_TO_SEEK:
        invocation.stderrChannel.print("failed to seek file\n");
        break;
    case errorCodes::XXD_CMD_ERROR_FAILED_TO_READ:
        invocation.stderrChannel.print("failed to read file\n");
        break;
    case errorCodes::XXD_CMD_ERROR_FAILED_TO_ALLOCATE_BUFFER:
        invocation.stderrChannel.print("failed to allocate buffer\n");
        break;
    default:
        break;
    }

    return CommandResult::completed(static_cast<int>(errorCode));
}
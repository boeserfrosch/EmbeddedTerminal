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
        return _generateHexDump(content.c_str(), content.length()) + "\n";
    }

    char buffer[512];
    file.read(buffer, 512);
    file.close();
    ETString result = _generateHexDump(buffer, 512);
    return result + "\n ... output truncated, file is larger than 512 bytes\n";
}

ETString EmbeddedTerminal::cmd::xxd::usage(const ETString &keyword)
{
    return keyword + " [file] - Returns the content of the defined file as hex dump\n";
}

ETVector<ETString> EmbeddedTerminal::cmd::xxd::getSuggestions(const ETString &partial)
{
    // Delegate to FilePathCompleter
    FilePathCompleter completer(_dir);
    return completer.getSuggestions(partial);
}

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::xxd::execute(CommandInvocation &invocation)
{
    auto state = _handleCommandState(invocation);
    errorCodes::XXDCmdErrorCode code = _checkCommandState(state, invocation);

    if (code != errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_NONE)
    {
        return _error(code, invocation);
    }

    return _streamHexDump(invocation, state);
}

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::xxd::_streamHexDump(CommandInvocation &invocation, const XXDState &state)
{
    char chunkBuffer[CHUNK_SIZE];

    auto executionState = _handleKeyStrokes(invocation);

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
        auto file = _dir.open(state.path.c_str(), "r", false);
        auto code = _checkFile(file);
        if (code != errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_NONE)
        {
            file.close();
            return _error(code, invocation);
        }

        file.seek(effectivePosition);

        size_t readBytes = file.read(chunkBuffer, CHUNK_SIZE);
        if (readBytes == 0)
        {
            file.close();
            return _error(errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_FAILED_TO_READ, invocation);
        }
        file.close();

        invocation.stdoutChannel.print(_generateHexDump(chunkBuffer, readBytes, effectivePosition));
    }

    return CommandResult::completed(errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_NONE);
}

EmbeddedTerminal::cmd::xxd::HandleKeyStrokesResult EmbeddedTerminal::cmd::xxd::_handleKeyStrokes(CommandInvocation &invocation)
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
            auto file = _dir.open(invocation.context.variables[SESSION_KEY_PATH].c_str(), "r", false);
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
            return HandleKeyStrokesResult{true, false};
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
            return HandleKeyStrokesResult{true, false};
        }
        else if (input == "q")
        {
            // Quit
            return HandleKeyStrokesResult{false, true};
        }
        else if (input == "g")
        {
            // Go to beginning
            invocation.context.variables[SESSION_KEY_POS] = "0";
            return HandleKeyStrokesResult{true, false};
        }
        else if (input == "G")
        {
            // Go to end
            auto file = _dir.open(invocation.context.variables[SESSION_KEY_PATH].c_str(), "r", false);
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
            return HandleKeyStrokesResult{true, false};
        }
        else if (input.startsWith("o"))
        {
            ETString offsetStr = input.substr(1).trim();
            size_t offset = std::stoull(offsetStr.c_str(), nullptr, 16);
            invocation.context.variables[SESSION_KEY_POS] = toETString(offset);
            return HandleKeyStrokesResult{true, false};
        }
    }

    return HandleKeyStrokesResult{false, false};
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
ETString EmbeddedTerminal::cmd::xxd::_generateHexDump(const char *content, size_t length, size_t startOffset, size_t bytesPerLine)
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

EmbeddedTerminal::cmd::xxd::XXDState EmbeddedTerminal::cmd::xxd::_handleCommandState(CommandInvocation &invocation)
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

EmbeddedTerminal::cmd::errorCodes::XXDCmdErrorCode EmbeddedTerminal::cmd::xxd::_checkCommandState(const XXDState &state, CommandInvocation &invocation)
{
    if (state.path.empty())
    {
        return errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_INVALID_PATH;
    }
    if (!_dir.exists(state.path.c_str()))
    {
        return errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_FILE_NOT_FOUND;
    }
    if (_dir.isDirectory(state.path.c_str()))
    {
        return errorCodes::XXDCmdErrorCode::XXD_CMD_ERROR_IS_DIRECTORY;
    }
    auto file = _dir.open(state.path.c_str(), "r", false);
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

EmbeddedTerminal::cmd::errorCodes::XXDCmdErrorCode EmbeddedTerminal::cmd::xxd::_checkFile(ETFile &file)
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

EmbeddedTerminal::CommandResult EmbeddedTerminal::cmd::xxd::_error(errorCodes::XXDCmdErrorCode errorCode, CommandInvocation &invocation)
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
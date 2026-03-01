#include "commands/download.h"
#include "DefaultAutoCompleters.h"
#include <sstream>

using namespace EmbeddedTerminal;
using namespace EmbeddedTerminal::cmd;

static const size_t RAW_CHUNK = 384;                     // 384 % 3 == 0
static const size_t B64_CHUNK = (RAW_CHUNK / 3) * 4 + 1; // +1 für Null-Terminator
// Base64-Tabelle
static const char b64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

// Encodiere `inBuf[0..len-1]` (len % 3 == 0) nach Base64 in outBuf.
// outBuf muss mindestens (len/3*4+1) groß sein.
// Am Ende steht ein '\0'.
ETString base64encode(const unsigned char *data, size_t len)
{
    static const char TABLE[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    ETString out;
    out.reserve(((len + 2) / 3) * 4);

    size_t i = 0;
    // volle 3-Byte-Blöcke
    for (; i + 2 < len; i += 3)
    {
        unsigned long v = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        out.push_back(TABLE[(v >> 18) & 0x3F]);
        out.push_back(TABLE[(v >> 12) & 0x3F]);
        out.push_back(TABLE[(v >> 6) & 0x3F]);
        out.push_back(TABLE[v & 0x3F]);
    }
    // Rest und Padding
    size_t rem = len - i;
    if (rem > 0)
    {
        unsigned long v = data[i] << 16;
        if (rem == 2)
            v |= data[i + 1] << 8;
        out.push_back(TABLE[(v >> 18) & 0x3F]);
        out.push_back(TABLE[(v >> 12) & 0x3F]);
        if (rem == 2)
            out.push_back(TABLE[(v >> 6) & 0x3F]);
        else
            out.push_back('=');
        out.push_back('=');
    }
    return out;
}

/// Erzeugt den Header "SIZE <fileSize>\n"
ETString createHeader(unsigned long fileSize)
{
    return "SIZE " + toETString(fileSize) + "\n";
}

ETString sendFile(ETFile &file)
{
    if (!file)
    {
        return createHeader(0) + "EOF\n";
    }

    size_t fileSize = file.size();

    ETString result = createHeader(fileSize);
    ETVector<unsigned char> buf(RAW_CHUNK);
    unsigned long sent = 0;
    while (sent < fileSize)
    {
        size_t toRead = RAW_CHUNK;
        if (fileSize - sent < RAW_CHUNK)
        {
            toRead = fileSize - sent;
        }
        size_t actuallyRead = file.read(buf.data(), toRead);
        if (actuallyRead == 0)
            break;
        ETString b64 = base64encode(buf.data(), actuallyRead);
        result += b64;
        result += "\n";
        sent += actuallyRead;
    }

    result += "EOF\n";
    return result;
}

ETString cmd::download::trigger(const ETString &keyword, const ETString &additional)
{

    additional.trim();
    auto param = split(additional, " ");

    if (param.size() != 1)
    {
        return "Expected parameter\n";
    }
    if (!dir_.exists(param[0].c_str()))
    {
        return "file did not exist\n";
    }
    if (dir_.isDirectory(param[0].c_str()))
    {
        return "file is a directory\n";
    }

    auto file = dir_.open(param[0].trim(), FILE_MODE_READ, false);

    return sendFile(file);
}

CommandResult cmd::download::execute(CommandInvocation &invocation)
{
    downloadState state = handleState_(invocation);
    unsigned char code = checkState_(state, invocation);
    if (code != 0)
    {
        return error_(code, invocation);
    }

    auto file = dir_.open(state.path.c_str(), FILE_MODE_READ, false);
    if (!file.isOpen())
    {
        return error_(errorCodes::DOWNLOAD_CMD_ERROR_FAILED_TO_OPEN_FILE, invocation);
    }

    if (state.position == 0)
    {
        // First call, send initial header
        size_t fileSize = file.size();
        invocation.stdoutChannel.print(createHeader(fileSize));
    }
    // Output sub header and chunk if there are bytes left to read else output EOF
    // Sub header is the current chunk index (starting with 0) and the total number of chunks, e.g. "CHUNK 0/10\n"
    if (state.position >= file.size())
    {
        invocation.stdoutChannel.print("\nEOF\n");
        file.close();
        return success_(invocation);
    }

    size_t fileSize = file.size();
    size_t numChunks = ((fileSize + RAW_CHUNK - 1) / RAW_CHUNK) - 1;
    size_t chunkIndex = state.position / RAW_CHUNK;
    invocation.stdoutChannel.print("CHUNK " + toETString(chunkIndex) + "/" + toETString(numChunks) + "\n");
    auto result = processChunk_(file, state.position, invocation);
    file.close();

    if (result.error)
    {
        return error_(result.errorCode, invocation);
    }
    else if (result.hasMore)
    {
        return CommandResult::running(errorCodes::DOWNLOAD_CMD_ERROR_NONE);
    }
    else
    {
        return success_(invocation);
    }
}

ETString cmd::download::usage(const ETString &keyword)
{
    return "Download a specific file\n\n" +
           keyword + " [path] - Download the file under the given path\n If the file did not exists than just a filesize of zero will be return ed";
}

ETVector<ETString> cmd::download::getSuggestions(const ETString &partial)
{
    FilePathCompleter completer(dir_);
    return completer.getSuggestions(partial);
}

EmbeddedTerminal::cmd::download::downloadState cmd::download::handleState_(CommandInvocation &invocation)
{
    downloadState state;
    // Check if this is a continuation of an ongoing stream
    auto &vars = invocation.context.variables;
    auto pathIt = vars.find(SESSION_KEY_PATH);
    auto posIt = vars.find(SESSION_KEY_POS);

    ETString path;
    size_t filePos = 0;

    if (pathIt != vars.end())
    {
        // Continued stream
        path = pathIt->second;
        posIt = vars.find(SESSION_KEY_POS);
        if (posIt != vars.end())
        {
            filePos = std::stoull(posIt->second.c_str());
        }
    }
    else
    {
        // New stream request
        path = invocation.arguments.trim();

        // Store path for potential re-entry
        vars[SESSION_KEY_PATH] = path;
        vars[SESSION_KEY_POS] = "0";
    }

    downloadState result;
    result.path = path;
    result.position = filePos;
    return result;
}

unsigned char cmd::download::checkState_(const downloadState &state, CommandInvocation &invocation)
{
    if (state.path.empty())
    {
        invocation.context.variables.erase(SESSION_KEY_PATH);
        invocation.context.variables.erase(SESSION_KEY_POS);
        return errorCodes::DOWNLOAD_CMD_ERROR_INVALID_PATH;
    }
    if (!dir_.exists(state.path.c_str()))
    {
        invocation.context.variables.erase(SESSION_KEY_PATH);
        invocation.context.variables.erase(SESSION_KEY_POS);
        return errorCodes::DOWNLOAD_CMD_ERROR_FILE_NOT_FOUND;
    }
    if (dir_.isDirectory(state.path.c_str()))
    {
        invocation.context.variables.erase(SESSION_KEY_PATH);
        invocation.context.variables.erase(SESSION_KEY_POS);
        return errorCodes::DOWNLOAD_CMD_ERROR_IS_DIRECTORY;
    }
    return 0;
}

EmbeddedTerminal::cmd::download::processChunkResult cmd::download::processChunk_(ETFile &file, size_t filePos, CommandInvocation &invocation)
{
    if (!file.seek(filePos))
    {
        invocation.context.variables.erase(SESSION_KEY_PATH);
        invocation.context.variables.erase(SESSION_KEY_POS);
        return processChunkResult(false, true, errorCodes::DOWNLOAD_CMD_ERROR_FAILED_TO_SEEK);
    }

    size_t fileSize = file.size();
    if (filePos >= fileSize)
    {
        // EOF
        invocation.stdoutChannel.print("\nEOF\n");
        return processChunkResult(false, false, errorCodes::DOWNLOAD_CMD_ERROR_NONE);
    }

    ETVector<unsigned char> buf(RAW_CHUNK);
    size_t toRead = RAW_CHUNK;
    if (fileSize - filePos < RAW_CHUNK)
    {
        toRead = fileSize - filePos;
    }
    size_t actuallyRead = file.read(buf.data(), toRead);
    if (actuallyRead == 0)
    {
        // EOF or read error
        invocation.stdoutChannel.print("\nEOF\n");
        return processChunkResult(false, true, errorCodes::DOWNLOAD_CMD_ERROR_FAILED_TO_READ);
    }

    ETString b64 = base64encode(buf.data(), actuallyRead);
    invocation.stdoutChannel.print(b64 + "\n");

    // Update position for potential continuation
    invocation.context.variables[SESSION_KEY_POS] = toETString(filePos + actuallyRead);

    return processChunkResult(true, false, errorCodes::DOWNLOAD_CMD_ERROR_NONE);
}

CommandResult cmd::download::error_(size_t errorCode, CommandInvocation &invocation)
{
    using namespace errorCodes;
    invocation.context.variables.erase(SESSION_KEY_PATH);
    invocation.context.variables.erase(SESSION_KEY_POS);

    switch (errorCode)
    {
    case DOWNLOAD_CMD_ERROR_INVALID_PATH:
        invocation.stderrChannel.print("Expected parameter\n");
        break;
    case DOWNLOAD_CMD_ERROR_FILE_NOT_FOUND:
        invocation.stderrChannel.print("file did not exist\n");
        break;
    case DOWNLOAD_CMD_ERROR_IS_DIRECTORY:
        invocation.stderrChannel.print("file is a directory\n");
        break;
    case DOWNLOAD_CMD_ERROR_FAILED_TO_OPEN_FILE:
        invocation.stderrChannel.print("failed to open file\n");
        break;
    case DOWNLOAD_CMD_ERROR_FAILED_TO_SEEK:
        invocation.stderrChannel.print("failed to seek in file\n");
        break;
    case DOWNLOAD_CMD_ERROR_FAILED_TO_READ:
        invocation.stderrChannel.print("failed to read from file\n");
        break;
    default:
        invocation.stderrChannel.print("unknown error\n");
        break;
    }

    return CommandResult::completed(errorCode);
}

CommandResult cmd::download::success_(CommandInvocation &invocation)
{
    invocation.context.variables.erase(SESSION_KEY_PATH);
    invocation.context.variables.erase(SESSION_KEY_POS);
    return CommandResult::completed(errorCodes::DOWNLOAD_CMD_ERROR_NONE);
}